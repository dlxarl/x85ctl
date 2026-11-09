cmake_minimum_required(VERSION 3.24)
project(KeyboardControlCenter VERSION 0.3.0 LANGUAGES CXX)
if(APPLE)
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "")
  # Homebrew Qt 6.9.0 references the removed AGL framework on recent SDKs.
  set(WrapOpenGL_AGL "/System/Library/Frameworks/OpenGL.framework" CACHE FILEPATH "" FORCE)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Test)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi)
option(KCC_BUILD_TESTS "Build unit tests" ON)

# ---- core library (no UI) -------------------------------------------------
add_library(kcc_core STATIC
  src/protocol/X85Protocol.cpp src/protocol/X85Layout.cpp
  src/protocol/CommandBuilder.cpp src/protocol/ProtocolParser.cpp
  src/hid/HidDevice.cpp src/hid/DeviceDiscovery.cpp
  src/core/TransactionRunner.cpp src/core/DeviceConnection.cpp src/core/DeviceManager.cpp
  src/storage/ProfileRepository.cpp
  src/services/KeyCatalog.cpp src/services/LightingService.cpp
  src/services/KeyMappingService.cpp src/services/ProfileService.cpp
  src/services/MacroService.cpp src/services/KeyRecorder.cpp src/services/SnakeGame.cpp src/services/AppShell.cpp
  src/ui/viewmodels/KeyboardLayoutModel.cpp)
target_sources(kcc_core PRIVATE src/platform/GlobalHotkeys.h)   # header-only QObject: let AUTOMOC see it
target_include_directories(kcc_core PUBLIC src)
target_link_libraries(kcc_core PUBLIC Qt6::Core Qt6::Gui PkgConfig::HIDAPI)
if(APPLE)
  target_sources(kcc_core PRIVATE src/platform/macos/MacPlatform.cpp src/platform/macos/GlobalHotkeysMac.mm src/ui/platform/TrayMac.mm)
  target_link_libraries(kcc_core PUBLIC "-framework Carbon" "-framework AppKit")
elseif(WIN32)
  target_sources(kcc_core PRIVATE src/platform/windows/WinPlatform.cpp src/platform/GlobalHotkeysOther.cpp src/ui/platform/TrayOther.cpp)
else()
  target_sources(kcc_core PRIVATE src/platform/linux/LinuxPlatform.cpp src/platform/GlobalHotkeysOther.cpp src/ui/platform/TrayOther.cpp)
endif()

# ---- app ------------------------------------------------------------------
qt_add_executable(KeyboardControlCenter MACOSX_BUNDLE src/main.cpp)
if(APPLE)
  # App icon: the vendor app's own icon (RT_GROUP_ICON 107 in DeviceDriver.exe, 64 px) on a macOS tile.
  set(KCC_ICNS ${CMAKE_SOURCE_DIR}/ui/assets/icon/AppIcon.icns)
  set_source_files_properties(${KCC_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(KeyboardControlCenter PRIVATE ${KCC_ICNS})
  target_sources(KeyboardControlCenter PRIVATE src/ui/platform/WindowChromeMac.mm)
  target_link_libraries(KeyboardControlCenter PRIVATE "-framework AppKit")
else()
  target_sources(KeyboardControlCenter PRIVATE src/ui/platform/WindowChromeOther.cpp)
endif()
target_compile_definitions(KeyboardControlCenter PRIVATE KCC_VERSION="${PROJECT_VERSION}")
file(GLOB KCC_QML RELATIVE ${CMAKE_SOURCE_DIR} CONFIGURE_DEPENDS ui/qml/*.qml)
foreach(f ${KCC_QML})
  get_filename_component(n ${f} NAME)
  set_source_files_properties(${f} PROPERTIES QT_RESOURCE_ALIAS ${n})
endforeach()
set_source_files_properties(ui/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(ui/assets/x85.png PROPERTIES QT_RESOURCE_ALIAS assets/x85.png)
set_source_files_properties(ui/assets/icon/app-256.png PROPERTIES QT_RESOURCE_ALIAS assets/app-icon.png)
set_source_files_properties(ui/assets/icon/tray.png PROPERTIES QT_RESOURCE_ALIAS assets/tray.png)
qt_add_qml_module(KeyboardControlCenter URI KCC VERSION 1.0 QML_FILES ${KCC_QML}
                  RESOURCES ui/assets/x85.png ui/assets/icon/app-256.png ui/assets/icon/tray.png RESOURCE_PREFIX /)
target_link_libraries(KeyboardControlCenter PRIVATE kcc_core Qt6::Quick Qt6::QuickControls2)
set_target_properties(KeyboardControlCenter PROPERTIES
  OUTPUT_NAME "ATTACK SHARK Control Center"
  MACOSX_BUNDLE_GUI_IDENTIFIER org.kcc.KeyboardControlCenter
  MACOSX_BUNDLE_BUNDLE_NAME "ATTACK SHARK Control Center"
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
  MACOSX_BUNDLE_ICON_FILE AppIcon)

# ---- CLI for hardware verification ------------------------------------------
add_executable(x85ctl tools/x85ctl.cpp)
target_link_libraries(x85ctl PRIVATE kcc_core)

if(KCC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
