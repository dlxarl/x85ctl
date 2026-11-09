import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// HSV picker: saturation/value field, hue strip, HEX or RGB entry, preset swatches.
//   picked(c)    — every change, including each step of a drag
//   committed(c) — a finished choice (release, typed value, swatch)
ColumnLayout {
    id: cp
    property color color: "#ffffff"
    signal picked(color c)
    signal committed(color c)
    spacing: 10

    // The picker keeps its own HSV so hue survives grey/black and a drag never jumps.
    property real h: 0
    property real s: 0
    property real v: 1
    property bool dragging: false
    property string entry: "hex"            // hex | rgb

    function clamp(x) { return Math.max(0, Math.min(1, x)) }
    function syncFromColor() {
        if (dragging) return
        if (color.hsvSaturation > 0 && color.hsvValue > 0 && color.hsvHue >= 0) h = color.hsvHue
        s = color.hsvSaturation; v = color.hsvValue
        refreshFields()
    }
    function setHsv(nh, ns, nv) { h = nh; s = ns; v = nv; picked(Qt.hsva(h, s, v, 1)) }
    function choose(c) { picked(c); committed(c) }
    function hex2(n) { return ("0" + Math.round(n * 255).toString(16)).slice(-2) }
    function hexOf(c) { return ("#" + hex2(c.r) + hex2(c.g) + hex2(c.b)).toUpperCase() }
    function refreshFields() {
        if (!hexField.activeFocus) hexField.text = hexOf(color)
        if (!rField.activeFocus) rField.text = Math.round(color.r * 255)
        if (!gField.activeFocus) gField.text = Math.round(color.g * 255)
        if (!bField.activeFocus) bField.text = Math.round(color.b * 255)
    }
    function applyRgb() {
        const n = (f) => Math.max(0, Math.min(255, parseInt(f.text) || 0)) / 255
        choose(Qt.rgba(n(rField), n(gField), n(bField), 1))
    }
    onColorChanged: syncFromColor()
    Component.onCompleted: syncFromColor()

    // ---- saturation / value field ----
    Rectangle {
        id: field
        Layout.fillWidth: true; Layout.preferredHeight: 150; radius: 6
        gradient: Gradient { orientation: Gradient.Horizontal
            GradientStop { position: 0; color: "white" } GradientStop { position: 1; color: Qt.hsva(cp.h, 1, 1, 1) } }
        Rectangle { anchors.fill: parent; radius: 6
            gradient: Gradient { GradientStop { position: 0; color: "transparent" } GradientStop { position: 1; color: "black" } } }
        Rectangle { width: 16; height: 16; radius: 8; border.color: "white"; border.width: 2; color: Qt.hsva(cp.h, cp.s, cp.v, 1)
                    x: cp.s * field.width - 8; y: (1 - cp.v) * field.height - 8 }
        MouseArea {
            anchors.fill: parent; cursorShape: Qt.CrossCursor
            preventStealing: true                   // the panel's ScrollView must not take the drag
            function upd(m) { cp.setHsv(cp.h, cp.clamp(m.x / width), 1 - cp.clamp(m.y / height)) }
            onPressed: (m) => { cp.dragging = true; upd(m) }
            onPositionChanged: (m) => upd(m)
            onReleased: { cp.dragging = false; cp.committed(Qt.hsva(cp.h, cp.s, cp.v, 1)) }
            onCanceled: cp.dragging = false
        }
    }

    // ---- hue strip ----
    Rectangle {
        id: hue
        Layout.fillWidth: true; Layout.preferredHeight: 12; radius: 6
        gradient: Gradient { orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#ff0000" } GradientStop { position: 0.17; color: "#ffff00" }
            GradientStop { position: 0.33; color: "#00ff00" } GradientStop { position: 0.5; color: "#00ffff" }
            GradientStop { position: 0.67; color: "#0000ff" } GradientStop { position: 0.83; color: "#ff00ff" }
            GradientStop { position: 1.0; color: "#ff0000" } }
        Rectangle { width: 16; height: 16; radius: 8; y: -2; x: cp.h * hue.width - 8; color: Qt.hsva(cp.h, 1, 1, 1)
                    border.color: "white"; border.width: 2 }
        MouseArea {
            anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor
            preventStealing: true
            function upd(m) {
                // a hue change on grey/black would be invisible: give it some colour
                cp.setHsv(Math.min(0.999, cp.clamp((m.x - 6) / hue.width)), cp.s > 0.02 ? cp.s : 1, cp.v > 0.02 ? cp.v : 1)
            }
            onPressed: (m) => { cp.dragging = true; upd(m) }
            onPositionChanged: (m) => upd(m)
            onReleased: { cp.dragging = false; cp.committed(Qt.hsva(cp.h, cp.s, cp.v, 1)) }
            onCanceled: cp.dragging = false
        }
    }

    // ---- value entry ----
    RowLayout {
        Layout.fillWidth: true; spacing: 8
        Rectangle { width: 30; height: 30; radius: 6; color: cp.color; border.color: Theme.line }
        Segmented {
            Layout.preferredWidth: 86; implicitHeight: 30
            model: [{label: "HEX", value: "hex"}, {label: "RGB", value: "rgb"}]
            value: cp.entry; onActivated: (v) => cp.entry = v
        }
        TextField {
            id: hexField
            visible: cp.entry === "hex"
            Layout.fillWidth: true; implicitHeight: 30
            font.family: Theme.mono; font.pixelSize: 12; color: Theme.text; leftPadding: 8
            selectByMouse: true
            validator: RegularExpressionValidator { regularExpression: /#?[0-9A-Fa-f]{0,6}/ }
            background: Rectangle { radius: 6; color: Theme.bg; border.color: hexField.activeFocus ? Theme.accent : Theme.line }
            onEditingFinished: {
                let t = text.replace("#", "")
                if (t.length === 3) t = t[0] + t[0] + t[1] + t[1] + t[2] + t[2]
                if (t.length === 6) cp.choose("#" + t)
                else text = cp.hexOf(cp.color)
            }
            onActiveFocusChanged: if (!activeFocus) cp.refreshFields()
        }
        TextField { id: rField; visible: cp.entry === "rgb"; Layout.fillWidth: true; implicitHeight: 30
            placeholderText: "R"; font.family: Theme.mono; font.pixelSize: 12; color: Theme.text; leftPadding: 6
            horizontalAlignment: Text.AlignHCenter; selectByMouse: true; validator: IntValidator { bottom: 0; top: 255 }
            background: Rectangle { radius: 6; color: Theme.bg; border.color: rField.activeFocus ? Theme.accent : Qt.alpha("#ff5d5d", 0.5) }
            onEditingFinished: cp.applyRgb(); onActiveFocusChanged: if (!activeFocus) cp.refreshFields() }
        TextField { id: gField; visible: cp.entry === "rgb"; Layout.fillWidth: true; implicitHeight: 30
            placeholderText: "G"; font.family: Theme.mono; font.pixelSize: 12; color: Theme.text; leftPadding: 6
            horizontalAlignment: Text.AlignHCenter; selectByMouse: true; validator: IntValidator { bottom: 0; top: 255 }
            background: Rectangle { radius: 6; color: Theme.bg; border.color: gField.activeFocus ? Theme.accent : Qt.alpha("#3ddc5f", 0.5) }
            onEditingFinished: cp.applyRgb(); onActiveFocusChanged: if (!activeFocus) cp.refreshFields() }
        TextField { id: bField; visible: cp.entry === "rgb"; Layout.fillWidth: true; implicitHeight: 30
            placeholderText: "B"; font.family: Theme.mono; font.pixelSize: 12; color: Theme.text; leftPadding: 6
            horizontalAlignment: Text.AlignHCenter; selectByMouse: true; validator: IntValidator { bottom: 0; top: 255 }
            background: Rectangle { radius: 6; color: Theme.bg; border.color: bField.activeFocus ? Theme.accent : Qt.alpha("#2f8bff", 0.6) }
            onEditingFinished: cp.applyRgb(); onActiveFocusChanged: if (!activeFocus) cp.refreshFields() }
    }

    // ---- presets ----
    Flow {
        Layout.fillWidth: true; spacing: 6
        Repeater {
            model: ["#ffffff", "#ff2d2d", "#ff7a00", "#ffd000", "#3ddc5f", "#00e0c6", "#1fb6ff", "#2f5bff", "#8a3dff", "#ff2bd6"]
            delegate: Rectangle {
                width: 23; height: 23; radius: 5; color: modelData
                border.width: Qt.colorEqual(cp.color, modelData) ? 2 : 1
                border.color: Qt.colorEqual(cp.color, modelData) ? "white" : Theme.line
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: cp.choose(modelData) }
            }
        }
    }
}
