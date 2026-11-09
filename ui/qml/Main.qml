import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qt.labs.platform as Platform

ApplicationWindow {
    id: win
    width: 1360; height: 820; minimumWidth: 1100; minimumHeight: 640; visible: true
    title: "ATTACK SHARK Control Center"
    color: Theme.bg
    // No title bar: main.cpp (applyWindowChrome) extends the content under it; on macOS the
    // traffic-light buttons sit in the top bar, which also moves the window.
    readonly property bool mac: Qt.platform.os === "osx"
    readonly property bool fullScreen: visibility === Window.FullScreen   // no traffic lights in full screen
    // Qt ≥ 6.9 pads ApplicationWindow content by the safe area (the hidden title bar); draw under it.
    topPadding: 0; leftPadding: 0; rightPadding: 0; bottomPadding: 0

    // ---------------- native menu bar ----------------
    Platform.MenuBar {
        Platform.Menu {
            title: "Keyboard"
            Platform.MenuItem { text: "Writing to keyboard"; checkable: true; checked: Device.writesEnabled
                                onTriggered: Device.writesEnabled = !Device.writesEnabled }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Send profile to keyboard"; enabled: Device.ready && Device.writesEnabled
                                onTriggered: ProfileStore.applyToKeyboard() }
            Platform.MenuItem { text: "Read colours from keyboard"; enabled: Device.ready && !Device.busy
                                onTriggered: Lighting.readPerKeyFromDevice() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Save profile"; shortcut: "Ctrl+S"; onTriggered: ProfileStore.saveActive() }
        }
        Platform.Menu {
            title: "Go"
            Platform.MenuItem { text: "Home"; shortcut: "Ctrl+1"; onTriggered: win.go("Home") }
            Platform.MenuItem { text: "Lighting"; shortcut: "Ctrl+2"; onTriggered: win.go("Lighting") }
            Platform.MenuItem { text: "Assignments"; shortcut: "Ctrl+3"; onTriggered: win.go("Assignments") }
            Platform.MenuItem { text: "Macros"; shortcut: "Ctrl+4"; onTriggered: win.go("Macros") }
            Platform.MenuItem { text: "Profiles"; shortcut: "Ctrl+5"; onTriggered: win.go("Profiles") }
            Platform.MenuItem { text: "Snake"; shortcut: "Ctrl+6"; onTriggered: win.go("Snake") }
            Platform.MenuItem { text: "Device"; shortcut: "Ctrl+7"; onTriggered: win.go("Device") }
        }
        Platform.Menu {
            title: "Games"
            Platform.MenuItem { text: Snake.running ? "Stop Snake" : "Start Snake"; enabled: Snake.running || Snake.canPlay
                                onTriggered: Snake.toggle() }
        }
        Platform.Menu {
            title: "Window"
            Platform.MenuItem { text: "Minimize"; shortcut: "Ctrl+M"; onTriggered: win.showMinimized() }
            Platform.MenuItem { text: win.visibility === Window.Maximized ? "Restore" : "Zoom"
                                onTriggered: win.visibility === Window.Maximized ? win.showNormal() : win.showMaximized() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Close window (keeps running in the background)"; shortcut: "Ctrl+W"; onTriggered: win.hide() }
        }
        Platform.Menu {
            title: "Help"
            Platform.MenuItem { text: "About ATTACK SHARK Control Center"; role: Platform.MenuItem.AboutRole; onTriggered: about.open() }
        }
    }
    Dialog {
        id: about
        modal: true; anchors.centerIn: parent; width: 380; padding: 22
        background: Rectangle { radius: 12; color: Theme.surface; border.color: Theme.line }
        contentItem: ColumnLayout {
            spacing: 10
            Image { source: "qrc:/KCC/assets/app-icon.png"; Layout.preferredWidth: 72; Layout.preferredHeight: 72; Layout.alignment: Qt.AlignHCenter
                    fillMode: Image.PreserveAspectFit; smooth: true }
            Text { text: "ATTACK SHARK Control Center"; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold; Layout.alignment: Qt.AlignHCenter }
            Text { text: "Version " + Qt.application.version; color: Theme.dim; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
            Text { Layout.fillWidth: true; wrapMode: Text.Wrap; horizontalAlignment: Text.AlignHCenter; color: Theme.faint; font.pixelSize: 11; lineHeight: 1.3
                   text: "An unofficial configuration app for the ATTACK SHARK X85 keyboard. The protocol was decoded from the vendor software; it is not affiliated with ATTACK SHARK." }
            AppButton { text: "Close"; Layout.alignment: Qt.AlignHCenter; onClicked: about.close() }
        }
    }

    readonly property var sections: [
        { name: "Home",        icon: "home", page: "Dashboard.qml" },
        { name: "Lighting",    icon: "sun", page: "LightingStudio.qml" },
        { name: "Assignments", short: "Keys", icon: "keys", page: "KeyMapping.qml" },
        { name: "Macros",      icon: "macro", page: "MacroStudio.qml" },
        { name: "Profiles",    icon: "layers", page: "Profiles.qml" },
        { name: "Snake",       icon: "snake", page: "SnakeGame.qml" },
        { name: "Device",      icon: "info", page: "DeviceInfo.qml" }
    ]
    property int current: 1
    Component.onCompleted: if (SnapshotTheme !== "") current = 0
    function go(name) { for (let i = 0; i < sections.length; ++i) if (sections[i].name === name) current = i }

    // ---------------- top bar ----------------
    Rectangle {
        id: top
        width: parent.width; height: 52; color: Theme.rail
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
        // acts as the title bar: drag to move, double-click to zoom
        DragHandler { target: null; grabPermissions: PointerHandler.CanTakeOverFromAnything
                      onActiveChanged: if (active) win.startSystemMove() }
        TapHandler { onDoubleTapped: win.visibility === Window.Maximized ? win.showNormal() : win.showMaximized() }
        RowLayout {
            anchors { fill: parent; leftMargin: (win.mac && !win.fullScreen) ? 86 : 20; rightMargin: 16 } spacing: 14
            Text { text: "ATTACK SHARK"; color: Theme.dim; font.pixelSize: 12; font.letterSpacing: 2; font.weight: Font.DemiBold }
            Text { text: "X85"; color: Theme.text; font.pixelSize: 17; font.weight: Font.Bold; font.letterSpacing: 1 }
            Rectangle {
                radius: 10; height: 22; implicitWidth: st.implicitWidth + 26
                color: Qt.alpha(Device.ready ? Theme.ok : Theme.warn, 0.12)
                Row { anchors.centerIn: parent; spacing: 6
                    Rectangle { width: 7; height: 7; radius: 4; anchors.verticalCenter: parent.verticalCenter
                                color: Device.ready ? Theme.ok : Device.connected ? Theme.warn : Theme.faint }
                    Text { id: st; font.pixelSize: 11; color: Theme.text
                           text: Device.ready ? "Connected · USB" : Device.connected ? "Config interface unavailable" : "Not connected" }
                }
            }
            Item { Layout.fillWidth: true }
            Text { text: "PROFILE"; color: Theme.faint; font.pixelSize: 10; font.letterSpacing: 1.5; font.weight: Font.DemiBold }
            AppCombo {
                id: prof; Layout.preferredWidth: 200
                model: ProfileStore.profiles
                currentIndex: ProfileStore.profiles.indexOf(ProfileStore.active)
                onActivated: (i) => ProfileStore.activate(ProfileStore.profiles[i])
            }
            AppButton { text: "Save"; implicitHeight: 30; ToolTip.visible: hovered; ToolTip.text: "Overwrite “" + ProfileStore.active + "” with the current settings"
                        onClicked: ProfileStore.saveActive() }
            Item { Layout.preferredWidth: 10 }
            // keyboard write access: quick toggle (also in Device page)
            Rectangle {
                radius: 15; height: 30; implicitWidth: wl.implicitWidth + 50
                color: wma.containsMouse ? Theme.hover : Theme.raised
                border.color: Device.writesEnabled ? Qt.alpha(Theme.accent, 0.6) : Theme.line
                Row { anchors.centerIn: parent; spacing: 8
                    Rectangle { width: 26; height: 14; radius: 7; anchors.verticalCenter: parent.verticalCenter
                                color: Device.writesEnabled ? Theme.accent : Theme.line
                                Rectangle { width: 10; height: 10; radius: 5; y: 2; x: Device.writesEnabled ? 14 : 2; color: "white"
                                            Behavior on x { NumberAnimation { duration: 120 } } } }
                    Text { id: wl; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12; color: Theme.text
                           text: Device.writesEnabled ? "Writing to keyboard" : "Read-only" }
                }
                MouseArea { id: wma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: Device.writesEnabled = !Device.writesEnabled }
            }
        }
    }

    // ---------------- icon rail ----------------
    Rectangle {
        id: rail
        anchors { top: top.bottom; bottom: parent.bottom } width: 76; color: Theme.rail
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }
        Column {
            anchors.top: parent.top; anchors.topMargin: 12; width: parent.width; spacing: 4
            Repeater {
                model: win.sections
                delegate: Item {
                    readonly property bool on: win.current === index
                    width: rail.width; height: 64
                    Rectangle { anchors.left: parent.left; width: 3; height: 34; radius: 2; anchors.verticalCenter: parent.verticalCenter
                                color: Theme.accent; visible: on }
                    Column {
                        anchors.centerIn: parent; spacing: 4
                        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: modelData.icon; size: 22
                               color: on ? Theme.accent : rma.containsMouse ? Theme.text : Theme.dim }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: modelData.short || modelData.name; font.pixelSize: 10
                               font.letterSpacing: 0.4; color: on ? Theme.text : Theme.faint; font.weight: Font.DemiBold
                               font.capitalization: Font.AllUppercase }
                    }
                    MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: win.current = index }
                }
            }
        }
    }

    // ---------------- page ----------------
    Loader {
        id: pageLoader
        anchors { left: rail.right; right: parent.right; top: top.bottom; bottom: parent.bottom }
        source: win.sections[win.current].page
        onLoaded: fade.restart()
        NumberAnimation { id: fade; target: pageLoader.item; property: "opacity"; from: 0; to: 1; duration: 160 }
    }

    // ---------------- status toast ----------------
    Rectangle {
        id: toast
        property string msg: Device.busy ? "Sending to keyboard…" : Device.lastResult
        visible: opacity > 0
        opacity: Device.busy || shown.running ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        anchors { right: parent.right; bottom: parent.bottom; margins: 18 }
        width: tt.implicitWidth + 40; height: 34; radius: 17
        color: Theme.raised; border.color: Theme.line
        Row { anchors.centerIn: parent; spacing: 8
            Rectangle { width: 7; height: 7; radius: 4; anchors.verticalCenter: parent.verticalCenter
                        color: Device.busy ? Theme.accent : toast.msg.indexOf("sent") >= 0 || toast.msg.indexOf("Read") === 0 ? Theme.ok : Theme.danger }
            Text { id: tt; text: toast.msg; color: Theme.text; font.pixelSize: 12 }
        }
        Timer { id: shown; interval: 2600 }
        Connections { target: Device; function onBusyChanged() { if (!Device.busy && Device.lastResult !== "") shown.restart() } }
    }
}
