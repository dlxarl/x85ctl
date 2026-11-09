import QtQuick

// The real X85 photo with a clickable overlay per key (geometry: KbLayout, in photo pixels).
//   lightMode: "effect" (animated preview of Lighting), "perkey" (Lighting.perKey), "none"
//   keyBadge(k): optional text shown on a key (Assignments)
Item {
    id: stage
    property string lightMode: "effect"
    property var keyBadge: null
    property var keyDimmed: function(k) { return false }
    property int selected: -1             // keyIndex
    property bool paintDrag: false
    property var colorSource: null        // perkey mode: draw this {lightIndex: colour} map instead (games)
    property bool showDevice: false       // perkey mode: draw the board's own table instead of the preset
    property int revision: 0              // bump to re-evaluate keyBadge/keyDimmed
    signal keyClicked(var key)
    signal keyPainted(var key)
    signal strokeStarted()
    signal strokeEnded()

    readonly property real s: Math.min(width / KbLayout.canvasWidth, height / KbLayout.canvasHeight)
    property real t: 0
    property int hoverKey: -1

    // ---- effect preview (approximation of the on-board effect, for orientation only) ----
    readonly property int caps: Lighting.caps
    readonly property bool rainbow: !(caps & 0x10) || Lighting.colorful
    Timer {
        running: stage.visible && stage.lightMode === "effect" && Lighting.mode !== 0
        interval: 40; repeat: true
        onTriggered: stage.t += 0.04 * (0.4 + Lighting.speed * 0.32)
    }
    function rnd(k) { const v = Math.sin(k.lightIndex * 12.9898) * 43758.5453; return v - Math.floor(v) }
    function wave(x) { return 0.5 + 0.5 * Math.sin(x * Math.PI * 2) }
    function effect(k) {
        const m = Lighting.mode
        if (m === 0 || Lighting.brightness === 0) return { c: "black", a: 0 }
        const t = stage.t
        const cx = (k.x + k.w / 2) / KbLayout.canvasWidth, cy = (k.y + k.h / 2) / KbLayout.canvasHeight
        const d = Lighting.direction
        const along = d === 0 ? cx : d === 1 ? 1 - cx : d === 2 ? cy : 1 - cy
        let hue = cx, v = 1
        switch (m) {
        case 1: hue = cx; break
        case 2: v = Math.pow(wave(t * 0.3 + rnd(k) * 9), 40); break                       // single on (reactive)
        case 3: v = 1 - Math.pow(wave(t * 0.3 + rnd(k) * 9), 40); break                   // single off
        case 4: v = 0.15 + 0.85 * Math.pow(wave(t * 0.7 + rnd(k) * 7), 6); hue = rnd(k); break
        case 5: v = Math.pow(wave(cy * 1.5 - t * 0.6 + Math.floor(rnd(k) * 4) / 4), 4); hue = rnd(k); break
        case 6: hue = (cx * 0.8 + cy * 0.3); break
        case 7: v = 0.08 + 0.92 * wave(t * 0.25 - 0.25); hue = Math.floor(t * 0.25) * 0.17; break
        case 8: hue = t * 0.08; break
        case 9: { const r = Math.hypot(cx - 0.5, (cy - 0.5) * 0.46); v = 0.2 + 0.8 * wave(r * 3 - t * 0.6); hue = r - t * 0.1 } break
        case 10: hue = along - t * 0.15; v = 0.25 + 0.75 * wave(along * 2 - t * 0.5); break
        case 11: hue = along - t * 0.15; break
        case 12: hue = Math.atan2((cy - 0.5) * 0.46, cx - 0.5) / (Math.PI * 2) + t * 0.12; break
        case 13: case 14: case 15: { const r = Math.hypot(cx - 0.45, (cy - 0.55) * 0.46); v = Math.pow(wave(r * 2.5 - t * 0.5), 8); hue = r * 2 } break
        case 16: hue = along * 0.7 + cy * 0.3 - t * 0.12; break
        case 17: v = 0.15 + 0.85 * wave(t * 0.4 + cx); hue = cx - t * 0.05; break
        case 18: hue = (along + cy) * 0.5 - t * 0.12; break
        case 19: { const p = 0.5 + 0.5 * Math.sin(t * 0.8); v = Math.max(0.08, 1 - Math.abs(cx - p) * 6) } break
        }
        const c = stage.rainbow ? Qt.hsva(((hue % 1) + 1) % 1, 0.9, 1, 1) : Lighting.color
        const b = stage.colorSource ? 1.0 : 0.35 + 0.65 * Lighting.brightness / Lighting.brightnessMax
        return { c: c, a: Math.max(0, Math.min(1, v)) * b }
    }

    Item {
        id: canvas
        width: KbLayout.canvasWidth * stage.s; height: KbLayout.canvasHeight * stage.s
        anchors.centerIn: parent

        Image {
            anchors.fill: parent
            source: "qrc:/KCC/assets/x85.png"
            fillMode: Image.PreserveAspectFit; smooth: true; mipmap: true
        }

        Repeater {
            model: KbLayout.keys
            delegate: Item {
                id: key
                readonly property var k: modelData
                readonly property var light: stage.lightMode === "effect" ? (stage.t, Lighting.mode, Lighting.color, Lighting.brightness,
                                                                              Lighting.colorful, Lighting.direction, stage.effect(k))
                                            : stage.lightMode === "perkey" ? (Lighting.perKey, Lighting.deviceColors, stage.colorSource, perKeyLight())
                                            : null
                function perKeyLight() {
                    const src = stage.colorSource ? stage.colorSource : (stage.showDevice ? Lighting.deviceColors : Lighting.perKey)
                    const c = src[String(k.lightIndex)]
                    const b = 0.35 + 0.65 * Lighting.brightness / Lighting.brightnessMax
                    return c === undefined ? { c: "black", a: 0, off: true } : { c: c, a: b }
                }
                readonly property bool sel: stage.selected === k.keyIndex
                readonly property bool hov: stage.hoverKey === k.keyIndex
                readonly property string badge: stage.keyBadge ? (stage.revision, stage.keyBadge(k) || "") : ""
                x: k.x * stage.s; y: k.y * stage.s; width: k.w * stage.s; height: k.h * stage.s

                // unlit key (per-key mode: not coloured → dark on the keyboard)
                Rectangle {
                    anchors.fill: parent; radius: 5 * stage.s
                    color: "#000000"
                    opacity: key.light && key.light.off ? 0.55 : (stage.revision, stage.keyDimmed(key.k)) ? 0.5 : 0
                }
                // light: tint + inner glow
                Rectangle {
                    visible: key.light !== null && key.light.a > 0.01
                    anchors.fill: parent; anchors.margins: 1; radius: 5 * stage.s
                    color: key.light ? key.light.c : "transparent"
                    opacity: key.light ? 0.18 + 0.5 * key.light.a : 0
                }
                Rectangle {
                    visible: key.light !== null && key.light.a > 0.01
                    anchors.fill: parent; radius: 6 * stage.s
                    color: "transparent"; border.width: Math.max(1.5, 2.5 * stage.s)
                    border.color: key.light ? key.light.c : "transparent"
                    opacity: key.light ? 0.35 + 0.65 * key.light.a : 0
                }
                // remap badge
                Rectangle {
                    visible: key.badge !== ""
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 3 * stage.s }
                    height: Math.max(14, 20 * stage.s); radius: 3
                    color: Theme.accent
                    Text {
                        anchors.fill: parent; anchors.leftMargin: 2; anchors.rightMargin: 2
                        text: key.badge; color: Theme.accentInk; font.pixelSize: Math.max(8, 11 * stage.s); font.bold: true
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
                // hover / selection ring
                Rectangle {
                    anchors.fill: parent; anchors.margins: -2; radius: 7 * stage.s
                    color: "transparent"
                    border.width: key.sel ? 2 : 1
                    border.color: key.sel ? Theme.accent : "#ffffff"
                    opacity: key.sel ? 1 : key.hov ? 0.7 : 0
                    Behavior on opacity { NumberAnimation { duration: 90 } }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: stage.hoverKey >= 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
            function hit(mx, my) {
                const px = mx / stage.s, py = my / stage.s
                for (const k of KbLayout.keys)
                    if (px >= k.x && px <= k.x + k.w && py >= k.y && py <= k.y + k.h) return k
                return null
            }
            property var last: null
            onReleased: if (stage.paintDrag) stage.strokeEnded()
            onCanceled: if (stage.paintDrag) stage.strokeEnded()
            onPressed: (m) => {
                if (stage.paintDrag) stage.strokeStarted()
                const k = hit(m.x, m.y); last = k
                if (k) { stage.keyClicked(k); if (stage.paintDrag) stage.keyPainted(k) }
            }
            onPositionChanged: (m) => {
                const k = hit(m.x, m.y)
                stage.hoverKey = k ? k.keyIndex : -1
                if (stage.paintDrag && pressed && k && k !== last) { last = k; stage.keyPainted(k) }
            }
            onExited: stage.hoverKey = -1
        }
    }
}
