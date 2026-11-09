#!/usr/bin/env bash
# Builds a self-contained .app and .dmg in dist/. Signing: set CODESIGN_ID="Developer ID Application: …".
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh Release
QT_PREFIX="$(brew --prefix qt)"
# Deploy into a copy: running macdeployqt on build/ would mix bundled Qt with the next dev build.
mkdir -p dist
APP="dist/ATTACK SHARK Control Center.app"
rm -rf "$APP"
ditto "build/ATTACK SHARK Control Center.app" "$APP"
"$QT_PREFIX/bin/macdeployqt" "$APP" -qmldir=ui/qml -always-overwrite ${CODESIGN_ID:+-codesign="$CODESIGN_ID" -hardened-runtime -timestamp}
# Homebrew's macdeployqt misses @rpath dependencies that only other bundled libraries need
# (e.g. libbrotlicommon, QtDBus). Copy every unresolved dylib/framework, repeat until closed.
FW="$APP/Contents/Frameworks"
LIBDIRS=("$(brew --prefix)/lib" "$QT_PREFIX/lib")
binaries() {   # every Mach-O file (framework binaries are not always executable)
  find "$APP/Contents" -type f ! -name "*.qml" ! -name "*.qmltypes" ! -name qmldir ! -name "*.plist" ! -name "*.png" -print0 \
    | xargs -0 file | awk -F': ' '/Mach-O/ {print $1}'
}
bundle_dep() {   # $1 = dependency as written in a load command; prints its bundle-relative name, copies it if missing
  local dep="$1" rel src
  case "$dep" in
    @rpath/*) rel="${dep#@rpath/}" ;;
    /opt/*|/usr/local/*) rel="${dep#*/lib/}" ;;
    *) return 1 ;;
  esac
  if [ ! -e "$FW/$rel" ]; then
    if [[ "$rel" == *.framework/* ]]; then
      local fwname="${rel%%/*}"
      case "$dep" in /*) src="${dep%%.framework/*}.framework" ;; *) src="$QT_PREFIX/lib/$fwname" ;; esac
      [ -d "$src" ] || return 1
      ditto "$src" "$FW/$fwname"; rm -rf "$FW/$fwname/Headers" "$FW/$fwname"/Versions/*/Headers; chmod -R u+w "$FW/$fwname"
      install_name_tool -id "@rpath/$rel" "$FW/$rel" 2>/dev/null
    else
      src=""
      case "$dep" in /*) src="$dep" ;; *) for d in "${LIBDIRS[@]}"; do [ -e "$d/$rel" ] && { src="$d/$rel"; break; }; done ;; esac
      [ -n "$src" ] && [ -e "$src" ] || return 1
      cp -L "$src" "$FW/$rel"; chmod u+w "$FW/$rel"; install_name_tool -id "@rpath/$rel" "$FW/$rel" 2>/dev/null
    fi
    echo "bundled missing $rel" >&2; added=1
  fi
  echo "$rel"
}
for _ in $(seq 1 15); do
  added=0; before="$(ls "$FW" | wc -l)"
  while IFS= read -r bin; do
    while IFS= read -r dep; do
      rel="$(bundle_dep "$dep")" || continue
      [[ "$dep" == /* ]] && { install_name_tool -change "$dep" "@rpath/$rel" "$bin" 2>/dev/null; added=1; }
    done < <(otool -L "$bin" 2>/dev/null | awk 'NR>1 && ($1 ~ /^@rpath\// || $1 ~ /^\/(opt|usr\/local)\//) {print $1}')
  done < <(binaries)
  [ "$(ls "$FW" | wc -l)" != "$before" ] && added=1   # bundle_dep runs in a subshell; count new files instead
  [ "$added" = 0 ] && break
done
# Install names (ids) of bundled libraries must be bundle-relative too.
while IFS= read -r bin; do
  id="$(otool -D "$bin" 2>/dev/null | awk 'NR==2')"
  case "$id" in /opt/*|/usr/local/*) install_name_tool -id "@rpath/${id#*/lib/}" "$bin" 2>/dev/null ;; esac
done < <(binaries)
# Resolve @rpath inside the bundle only: drop rpaths into Homebrew, point the executable at Frameworks.
while IFS= read -r bin; do
  for r in $(otool -l "$bin" 2>/dev/null | awk '/LC_RPATH/{getline;getline;print $2}' | grep -E "^/(opt|usr/local)/"); do
    install_name_tool -delete_rpath "$r" "$bin" 2>/dev/null
  done
done < <(binaries)
EXE="$APP/Contents/MacOS/ATTACK SHARK Control Center"
otool -l "$EXE" | grep -q "@executable_path/../Frameworks" || install_name_tool -add_rpath @executable_path/../Frameworks "$EXE" 2>/dev/null
# Fail loudly if anything still points outside the bundle.
if binaries | while IFS= read -r b; do otool -L "$b" 2>/dev/null | awk 'NR>1' ; done | grep -E "^[[:space:]]+/(opt|usr/local)/"; then
  echo "error: bundle still references libraries outside the app" >&2; exit 1
fi

# macdeployqt rewrites binaries, which invalidates signatures; Apple Silicon kills unsigned code.
if [ -z "${CODESIGN_ID:-}" ]; then
  codesign --force --deep --sign - "$APP"          # ad-hoc: runs locally, not for distribution
fi
codesign --verify --deep --strict "$APP"
# Smoke test: the bundled app must at least load all its libraries (dyld fails before main otherwise).
"$APP/Contents/MacOS/ATTACK SHARK Control Center" --kcc-smoke-test || { echo "error: bundled app does not start" >&2; exit 1; }
hdiutil create -volname "ATTACK SHARK Control Center" -srcfolder "$APP" -ov -format UDZO "dist/ATTACK-SHARK-Control-Center.dmg"
echo "dist/ATTACK-SHARK-Control-Center.dmg"
