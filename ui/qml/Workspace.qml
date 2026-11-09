import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Shapes

// G HUB-style page: settings panel on the left, device stage on the right.
//   default children → scrollable panel body;  stage: [...] → centre;  footer: [...] → pinned panel bottom
Item {
    id: ws
    property string title
    property var tabs: []                 // [{label, value}]
    property var tab
    signal tabActivated(var value)
    default property alias panel: body.data
    property alias stage: stageArea.data
    property alias footer: foot.data
    property string hint: ""              // short caption under the stage

    Rectangle {
        id: side
        width: Theme.panelWidth; height: parent.height
        color: Theme.surface
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }

        ColumnLayout {
            anchors.fill: parent; spacing: 0
            Text {
                Layout.leftMargin: 22; Layout.topMargin: 20; Layout.bottomMargin: ws.tabs.length ? 12 : 16
                text: ws.title; color: Theme.text; font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase; font.letterSpacing: 1.2
            }
            Row {   // underline tabs, like G HUB
                visible: ws.tabs.length > 0
                Layout.leftMargin: 22; Layout.bottomMargin: 6; spacing: 22
                Repeater {
                    model: ws.tabs
                    delegate: Item {
                        readonly property bool on: modelData.value === ws.tab
                        width: tl.implicitWidth; height: 30
                        Text { id: tl; text: modelData.label; anchors.verticalCenter: parent.verticalCenter
                               font.pixelSize: 12; font.weight: Font.DemiBold; font.letterSpacing: 1
                               font.capitalization: Font.AllUppercase
                               color: on ? Theme.text : tma.containsMouse ? Theme.dim : Theme.faint }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; radius: 1
                                    color: Theme.accent; visible: on }
                        MouseArea { id: tma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: ws.tabActivated(modelData.value) }
                    }
                }
            }
            Rectangle { visible: ws.tabs.length > 0; Layout.fillWidth: true; height: 1; color: Theme.line }
            ScrollView {
                id: sv
                Layout.fillWidth: true; Layout.fillHeight: true
                contentWidth: availableWidth; clip: true
                ColumnLayout {
                    id: body
                    x: 22; y: 16
                    width: sv.availableWidth - 44
                    spacing: 14
                }
                contentHeight: body.implicitHeight + 32
            }
            Rectangle { visible: foot.children.length > 0; Layout.fillWidth: true; height: 1; color: Theme.line }
            ColumnLayout {
                id: foot
                visible: children.length > 0
                Layout.fillWidth: true; Layout.margins: 16; Layout.leftMargin: 22; Layout.rightMargin: 22
                spacing: 10
            }
        }
    }

    Item {
        anchors { left: side.right; right: parent.right; top: parent.top; bottom: parent.bottom }
        clip: true
        Shape {   // soft spotlight behind the device
            id: spot
            anchors.fill: parent
            ShapePath {
                strokeWidth: -1
                fillGradient: RadialGradient {
                    centerX: spot.width / 2; centerY: spot.height * 0.45; centerRadius: Math.max(spot.width, spot.height) * 0.62
                    focalX: centerX; focalY: centerY
                    GradientStop { position: 0.0; color: Theme.stageHi }
                    GradientStop { position: 1.0; color: Theme.bg }
                }
                startX: 0; startY: 0
                PathLine { x: spot.width; y: 0 }
                PathLine { x: spot.width; y: spot.height }
                PathLine { x: 0; y: spot.height }
                PathLine { x: 0; y: 0 }
            }
        }
        Item { id: stageArea; anchors.fill: parent; anchors.margins: 36; anchors.bottomMargin: 64 }
        Text {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 26 }
            text: ws.hint; color: Theme.dim; font.pixelSize: 12
        }
    }
}
