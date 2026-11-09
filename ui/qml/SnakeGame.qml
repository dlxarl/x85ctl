import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Snake on the keyboard: every key is a cell, steer with the arrow keys. It is drawn on the photo and,
// if you switch it on, on the real LEDs. When the game ends the previous lighting is sent back.
Workspace {
    id: page
    title: "Snake"
    focus: true
    hint: Snake.running ? "Arrow keys steer · Esc stops"
                        : Snake.over ? "Game over — lighting restored" : "Press Start (or your shortcut), then steer with the arrow keys"

    Component.onDestruction: Snake.stop()
    Component.onCompleted: page.forceActiveFocus()

    Keys.onPressed: (e) => {
        if (e.isAutoRepeat) return
        if (e.key === Qt.Key_Up) Snake.turn(0)
        else if (e.key === Qt.Key_Right) Snake.turn(1)
        else if (e.key === Qt.Key_Down) Snake.turn(2)
        else if (e.key === Qt.Key_Left) Snake.turn(3)
        else if (e.key === Qt.Key_Escape && Snake.running) Snake.stop()
        else return
        e.accepted = true
    }

    stage: Item {
        anchors.fill: parent
        KeyboardStage {
            anchors.fill: parent
            lightMode: "perkey"
            colorSource: Snake.colors
            enabled: false
        }
        // score card over the stage
        Row {
            anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: -8 }
            spacing: 28
            Repeater {
                model: [{k: "SCORE", v: Snake.score}, {k: "LENGTH", v: Snake.running || Snake.over ? Snake.length : 0}, {k: "BEST", v: Snake.best}]
                delegate: Column { spacing: 2
                    Text { text: modelData.k; color: Theme.faint; font.pixelSize: 10; font.letterSpacing: 1.5; font.weight: Font.DemiBold
                           anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: modelData.v; color: Theme.text; font.pixelSize: 26; font.weight: Font.DemiBold
                           anchors.horizontalCenter: parent.horizontalCenter }
                }
            }
        }
    }

    SectionLabel { text: "Game" }
    AppButton {
        Layout.fillWidth: true; primary: !Snake.running
        enabled: Snake.running || Snake.canPlay
        text: Snake.running ? "Stop" : Snake.over ? "Play again" : "Start"
        onClicked: { Snake.toggle(); page.forceActiveFocus() }
    }
    Text { visible: !Snake.canPlay && !Snake.running; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.warn; font.pixelSize: 12
           text: Device.ready ? "Switch on “Writing to keyboard” (top right) to play." : "Connect the keyboard to play." }
    Text { visible: Snake.message !== ""; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.warn; font.pixelSize: 12; text: Snake.message }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim; font.pixelSize: 12; lineHeight: 1.3
           text: "The game is played on the keyboard's own LEDs — the photo just mirrors it. You start as one key; each red key adds one. The arrow keys are lit violet. You pass through the edges and come out on the opposite side; only your own tail ends the game. When it ends, your lighting comes back." }

    SectionLabel { text: "Shortcut" }
    RowLayout {
        Layout.fillWidth: true; spacing: 8
        Rectangle {
            Layout.fillWidth: true; height: 34; radius: Theme.radius; color: Theme.bg
            border.color: Snake.capturing ? Theme.accent : Theme.line
            Text { anchors.centerIn: parent; font.pixelSize: 13
                   color: Snake.capturing ? Theme.accent : Snake.hotkeyLabel ? Theme.text : Theme.faint
                   text: Snake.capturing ? "Press the combination…" : Snake.hotkeyLabel ? Snake.hotkeyLabel : "Not set" }
        }
        AppButton { text: Snake.capturing ? "Cancel" : "Set"; enabled: Snake.hotkeySupported
                    onClicked: Snake.capturing ? Snake.cancelCapture() : Snake.captureHotkey() }
        AppButton { text: "Clear"; enabled: Snake.hotkeyLabel !== "" && !Snake.capturing; onClicked: Snake.clearHotkey() }
    }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11; lineHeight: 1.3
           text: Snake.hotkeySupported
                 ? "Press the shortcut anywhere to start the game, and again to stop it. Use at least one of ⌘ ⌃ ⌥ ⇧. While the game runs, the arrow keys steer it in any app and Esc stops it; they work normally again afterwards."
                 : "System-wide shortcuts are only available on macOS for now. Use the Start button." }

    SectionLabel { text: "Speed" }
    AppSlider {
        Layout.fillWidth: true; label: "Step"; from: 150; to: 1000; stepSize: 25; suffix: " ms"
        value: Snake.intervalMs; onMoved: (v) => { Snake.intervalMs = v; page.forceActiveFocus() }
    }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11; lineHeight: 1.3
           text: "A full LED update takes about half a second, so the keyboard trails the game slightly; a step of 400 ms or more feels best. Every update is saved by the keyboard (04 F0), so a game is a few hundred writes." }
}
