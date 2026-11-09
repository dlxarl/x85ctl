import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Workspace {
    id: page
    title: "Lighting"
    // The X85 shows one thing at a time: an effect, or the per-key colour table (user mode 0x80).
    // So "Per-key" is one entry in the same mode list; the table is kept when an effect is chosen.
    readonly property bool perKey: Lighting.perKeyActive
    hint: perKey ? "Click or drag across keys to paint · colour and tools are on the left"
                 : "Preview is approximate — the keyboard renders the effect itself"

    property color brush: "#1fb6ff"
    property string tool: "brush"          // brush | eraser | picker
    property var recent: []
    readonly property int caps: Lighting.caps
    readonly property bool canWrite: Device.ready && Device.writesEnabled
    function remember(c) {                  // "recent" = colours actually used, not every drag step
        const hex = c.toString()
        recent = [hex].concat(recent.filter(x => x !== hex)).slice(0, 10)
    }
    function chooseEffect(id) { Lighting.mode = id; Lighting.perKeyActive = false }

    stage: KeyboardStage {
        anchors.fill: parent
        // Effect mode: always the effect preview. While an effect runs, 04 F5 returns a frozen frame of the
        // animation (odd colours), so the board's table is shown only in per-key mode, until you edit.
        lightMode: page.perKey ? "perkey" : "effect"
        showDevice: page.perKey && Lighting.deviceShown
        paintDrag: page.perKey
        onStrokeStarted: { Lighting.beginStroke(); if (page.tool === "brush") page.remember(page.brush) }
        onStrokeEnded: Lighting.endStroke()
        onKeyPainted: (k) => {
            if (!page.perKey) return
            if (page.tool === "brush") Lighting.setKeyColor(k.lightIndex, page.brush)
            else if (page.tool === "eraser") Lighting.clearKey(k.lightIndex)
            else { const c = Lighting.perKey[String(k.lightIndex)]; if (c !== undefined) { page.brush = c; page.tool = "brush" } }
        }
    }

    // ================= MODE (compact: current mode + expandable list) =================
    property bool modesOpen: false
    readonly property string modeName: perKey ? "Per-key colours"
        : ((Lighting.modes.find(m => m.id === Lighting.mode) || {name: "—"}).name)
    SectionLabel { text: "Mode" }
    Rectangle {
        Layout.fillWidth: true; height: 40; radius: Theme.radius
        color: hma.containsMouse ? Theme.hover : Theme.raised
        border.color: page.modesOpen ? Theme.accent : Theme.line
        Row {
            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
            spacing: 9
            Icon { name: page.perKey ? "grid" : "spark"; size: 16; anchors.verticalCenter: parent.verticalCenter; color: Theme.accent }
            Text { text: page.modeName; font.pixelSize: 13; font.weight: Font.DemiBold; color: Theme.text
                   anchors.verticalCenter: parent.verticalCenter }
        }
        Text { anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
               text: page.modesOpen ? "Hide ▴" : "Change ▾"; color: Theme.faint; font.pixelSize: 11 }
        MouseArea { id: hma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: page.modesOpen = !page.modesOpen }
    }
    ColumnLayout {
        visible: page.modesOpen; Layout.fillWidth: true; spacing: 6
        Rectangle {   // per-key colours: the keyboard's user mode
            Layout.fillWidth: true; height: 36; radius: Theme.radius
            color: page.perKey ? Qt.alpha(Theme.accent, 0.16) : pma.containsMouse ? Theme.hover : Theme.raised
            border.color: page.perKey ? Theme.accent : "transparent"
            Row {
                anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                spacing: 9
                Icon { name: "grid"; size: 15; anchors.verticalCenter: parent.verticalCenter
                       color: page.perKey ? Theme.accent : Theme.dim }
                Text { text: "Per-key colours"; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter
                       color: page.perKey ? Theme.text : Theme.dim; font.weight: page.perKey ? Font.DemiBold : Font.Normal }
            }
            Text { anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                   text: KbLayout.keys.filter(k => Lighting.perKey[String(k.lightIndex)] !== undefined).length + " keys"
                   color: Theme.faint; font.pixelSize: 11 }
            MouseArea { id: pma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: { Lighting.perKeyActive = true; page.modesOpen = false } }
        }
        GridLayout {
            Layout.fillWidth: true; columns: 2; columnSpacing: 6; rowSpacing: 6
            Repeater {
                model: Lighting.modes
                delegate: Rectangle {
                    readonly property bool on: !page.perKey && modelData.id === Lighting.mode
                    Layout.fillWidth: true; height: 32; radius: Theme.radius
                    color: on ? Qt.alpha(Theme.accent, 0.16) : ema.containsMouse ? Theme.hover : Theme.raised
                    border.color: on ? Theme.accent : "transparent"
                    Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                           text: modelData.name; font.pixelSize: 13; color: on ? Theme.text : Theme.dim
                           font.weight: on ? Font.DemiBold : Font.Normal }
                    MouseArea { id: ema; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: { page.chooseEffect(modelData.id); page.modesOpen = false } }
                }
            }
        }
    }
    Text { visible: page.modesOpen; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11
           text: "The X85 runs one mode at a time: an effect, or your per-key colours. Your per-key colours stay saved while an effect is on." }

    // ================= EFFECT settings =================
    SectionLabel { visible: !page.perKey && (page.caps & 0x03); text: "Parameters" }
    AppSlider { visible: !page.perKey && (page.caps & 0x01); Layout.fillWidth: true; label: "Brightness"
                from: 0; to: Lighting.brightnessMax; value: Lighting.brightness; onMoved: (v) => Lighting.brightness = v }
    AppSlider { visible: !page.perKey && (page.caps & 0x02); Layout.fillWidth: true; label: "Speed"
                from: 0; to: Lighting.speedMax; value: Lighting.speed; onMoved: (v) => Lighting.speed = v }
    RowLayout {
        visible: !page.perKey && (page.caps & 0x0C); spacing: 12
        Text { text: "Direction"; color: Theme.dim; font.pixelSize: Theme.fontBody; Layout.preferredWidth: 76 }
        Segmented {
            Layout.fillWidth: true
            model: (page.caps & 0x04) ? [{label: "← Left", value: 0}, {label: "Right →", value: 1}]
                                      : [{label: "↑ Up", value: 2}, {label: "Down ↓", value: 3}]
            value: Lighting.direction; onActivated: (v) => Lighting.direction = v
        }
    }
    SectionLabel { visible: !page.perKey && (page.caps & 0x30); text: "Colour" }
    Segmented {
        visible: !page.perKey && (page.caps & 0x20); Layout.fillWidth: true
        model: [{label: "Single colour", value: false}, {label: "Rainbow", value: true}]
        value: Lighting.colorful; onActivated: (v) => Lighting.colorful = v
    }
    ColorPicker {
        Layout.fillWidth: true
        visible: !page.perKey && (page.caps & 0x10) && !Lighting.colorful
        color: Lighting.color; onPicked: (c) => Lighting.color = c
    }
    Text { visible: !page.perKey && Lighting.mode === 0; text: "Backlight off."
           color: Theme.dim; font.pixelSize: Theme.fontBody }
    Text { visible: !page.perKey && Lighting.mode !== 0 && !(page.caps & 0x30)
           text: "This effect uses its own colours."; color: Theme.dim; font.pixelSize: Theme.fontBody }

    // ================= PER-KEY editor =================
    SectionLabel { visible: page.perKey; text: "Tool" }
    Segmented {
        visible: page.perKey; Layout.fillWidth: true
        model: [{label: "Paint", icon: "brush", value: "brush"}, {label: "Erase", icon: "eraser", value: "eraser"},
                {label: "Pick", icon: "pipette", value: "picker"}]
        value: page.tool; onActivated: (v) => page.tool = v
    }
    SectionLabel { visible: page.perKey; text: "Colour" }
    ColorPicker {
        visible: page.perKey; Layout.fillWidth: true
        color: page.brush
        onPicked: (c) => { page.brush = c; if (page.tool !== "brush") page.tool = "brush" }
    }
    RowLayout {
        visible: page.perKey && page.recent.length > 0; spacing: 6
        Text { text: "Recent"; color: Theme.faint; font.pixelSize: 11; Layout.preferredWidth: 46 }
        Flow {
            Layout.fillWidth: true; spacing: 5
            Repeater {
                model: page.recent
                delegate: Rectangle { width: 19; height: 19; radius: 4; color: modelData; border.color: Theme.line
                                      MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                                  onClicked: { page.brush = modelData; page.tool = "brush" } } }
            }
        }
    }
    SectionLabel { visible: page.perKey; text: "Whole keyboard" }
    RowLayout {
        visible: page.perKey; spacing: 8; Layout.fillWidth: true
        AppButton { text: "Fill all"; Layout.fillWidth: true; onClicked: { page.remember(page.brush); Lighting.fillAll(page.brush) } }
        AppButton { text: "Clear all"; Layout.fillWidth: true; onClicked: Lighting.clearPerKey() }
        AppButton { text: "Read"; Layout.fillWidth: true; enabled: Device.ready && !Device.busy
                    onClicked: Lighting.readPerKeyFromDevice() }
    }
    AppSlider { visible: page.perKey; Layout.fillWidth: true; label: "Brightness"
                from: 0; to: Lighting.brightnessMax; value: Lighting.brightness; onMoved: (v) => Lighting.brightness = v }
    Text { visible: page.perKey; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11
           text: KbLayout.keys.filter(k => Lighting.perKey[String(k.lightIndex)] !== undefined).length + " of " + KbLayout.keys.length + " keys coloured. Keys without a colour stay dark." }

    // ================= what the editor shows =================
    SectionLabel { text: "Keyboard state" }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim; font.pixelSize: 11
           text: (Lighting.syncInfo ? Lighting.syncInfo + ". " : "Per-key colours are read from the keyboard when it connects. ")
                 + "Effect, brightness and speed cannot be read from the X85 (the vendor protocol has no command for it), "
                 + "so the app shows what it last set; every change is saved automatically." }

    // ================= colour accuracy (both tabs) =================
    SectionLabel { text: "Colour accuracy" }
    AppSwitch { text: "Correct colours for the keyboard LEDs"; checked: Lighting.correctionEnabled
                onToggled: Lighting.correctionEnabled = checked }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11
           text: "LEDs are linear and their green and blue are much brighter than red, so raw colours look pale and greenish. Gamma turns screen colours into LED brightness; the R/G/B levels set the white balance." }
    ColumnLayout {
        visible: Lighting.correctionEnabled; Layout.fillWidth: true; spacing: 6
        AppSlider { Layout.fillWidth: true; label: "Gamma"; from: 1.0; to: 3.0; stepSize: 0.1; decimals: 1
                    value: Lighting.gamma; onMoved: (v) => Lighting.gamma = v }
        AppSlider { Layout.fillWidth: true; label: "Red"; from: 20; to: 100; suffix: "%"
                    value: Math.round(Lighting.gainR * 100); onMoved: (v) => Lighting.gainR = v / 100 }
        AppSlider { Layout.fillWidth: true; label: "Green"; from: 20; to: 100; suffix: "%"
                    value: Math.round(Lighting.gainG * 100); onMoved: (v) => Lighting.gainG = v / 100 }
        AppSlider { Layout.fillWidth: true; label: "Blue"; from: 20; to: 100; suffix: "%"
                    value: Math.round(Lighting.gainB * 100); onMoved: (v) => Lighting.gainB = v / 100 }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            AppButton { text: Lighting.calibrating ? "Done" : "Calibrate white"; primary: Lighting.calibrating
                        Layout.fillWidth: true; enabled: page.canWrite
                        onClicked: Lighting.calibrating = !Lighting.calibrating }
            AppButton { text: "Defaults"; onClicked: Lighting.resetCorrection() }
        }
        Text { visible: Lighting.calibrating; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.warn; font.pixelSize: 11
               text: "The keyboard now shows white. Lower the channel it is tinted with (usually green, then blue) until the white looks neutral, then press Done." }
    }

    // ================= footer =================
    footer: [
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            AppSwitch { text: "Live"; checked: Lighting.autoApply; enabled: page.canWrite
                        onToggled: Lighting.autoApply = checked }
            Item { Layout.fillWidth: true }
            AppButton { text: page.perKey ? "Apply colours" : "Apply effect"; primary: true
                        enabled: page.canWrite; onClicked: Lighting.applyCurrent() }
        },
        WriteGate { Layout.fillWidth: true }
    ]
}
