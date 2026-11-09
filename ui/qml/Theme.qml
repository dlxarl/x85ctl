pragma Singleton
import QtQuick

// Dark, G HUB-like palette. (`override`/`mode` are kept for the snapshot tool.)
QtObject {
    property string override: ""
    readonly property string mode: "dark"
    function setMode(m) {}
    readonly property bool dark: true

    readonly property color bg:        "#0b0c0f"   // stage
    readonly property color stageHi:   "#1a1d24"   // stage spotlight
    readonly property color rail:      "#08090b"
    readonly property color surface:   "#131519"   // side panel
    readonly property color raised:    "#1b1e24"   // rows / inputs
    readonly property color hover:     "#22262e"
    readonly property color line:      "#262a31"
    readonly property color text:      "#eef0f3"
    readonly property color dim:       "#8b919c"
    readonly property color faint:     "#545a64"
    readonly property color accent:    "#1fb6ff"
    readonly property color accentInk: "#04131c"
    readonly property color ok:        "#3ddc97"
    readonly property color warn:      "#f2b84b"
    readonly property color danger:    "#ff5d5d"
    readonly property color keycap:    "#1b1e24"
    readonly property color keyLine:   "#3a3f48"

    readonly property int radius: 6
    readonly property int gap: 14
    readonly property string mono: Qt.platform.os === "osx" ? "Menlo" : "monospace"
    readonly property int fontSmall: 11
    readonly property int fontBody: 13
    readonly property int fontTitle: 15
    readonly property int panelWidth: 340
}
