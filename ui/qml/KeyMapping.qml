import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Workspace {
    id: page
    title: "Assignments"
    tabs: [{label: "Keys", value: "key"}, {label: "Media", value: "media"}, {label: "Mouse", value: "mouse"}, {label: "Macro", value: "macro"}]
    tab: "key"
    onTabActivated: (v) => tab = v
    hint: (rev, sel) ? "Selected: " + sel.label + (KeyMap.describe(sel.keyIndex) ? "  →  " + KeyMap.describe(sel.keyIndex) : "  (default)")
                + " · choose an action on the left"
              : "Click a key on the keyboard, then choose what it should do"

    property var sel: null
    property int rev: 0
    property string filter: ""
    readonly property int mods: (mCtrl.checked ? 1 : 0) | (mShift.checked ? 2 : 0) | (mAlt.checked ? 4 : 0) | (mWin.checked ? 8 : 0)
    Connections { target: KeyMap; function onChanged() { page.rev++ } }

    property int macroMode: 0
    property int macroCount: 3
    function assignMacro(i) { if (sel) KeyMap.assignMacro(sel.keyIndex, i, macroMode, macroCount) }
    function assign(item) {
        if (!sel) return
        if (tab === "key") KeyMap.assignKey(sel.keyIndex, item.usage, mods)
        else if (tab === "media") KeyMap.assignMedia(sel.keyIndex, item.usage)
        else KeyMap.assignMouse(sel.keyIndex, item.kind, item.value)
    }
    readonly property var items: {
        if (tab === "macro") return []
        const src = tab === "key" ? KeyMap.keyTargets : tab === "media" ? KeyMap.mediaTargets : KeyMap.mouseTargets
        const f = filter.toLowerCase()
        return f ? src.filter(x => x.name.toLowerCase().indexOf(f) >= 0) : src
    }

    stage: KeyboardStage {
        anchors.fill: parent
        lightMode: "none"
        revision: page.rev + KeyMap.layer * 100000
        selected: page.sel ? page.sel.keyIndex : -1
        keyBadge: function(k) { const l = KeyMap.labels[String(k.keyIndex)]; return l !== undefined ? l : "" }
        keyDimmed: function(k) { return !k.remappable }
        onKeyClicked: (k) => { if (k.remappable) page.sel = k }
    }

    SectionLabel { text: "Layer" }
    Segmented {
        Layout.fillWidth: true
        model: [{label: "Standard", value: 0}, {label: "Fn layer", value: 1}]
        value: KeyMap.layer; onActivated: (v) => KeyMap.layer = v
    }

    Rectangle {   // selected key card
        Layout.fillWidth: true; implicitHeight: selCol.implicitHeight + 24; radius: Theme.radius
        color: Theme.raised; border.color: page.sel ? Qt.alpha(Theme.accent, 0.5) : Theme.line
        ColumnLayout {
            id: selCol; anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 } spacing: 6
            RowLayout {
                spacing: 10
                Rectangle { width: 38; height: 38; radius: 6; color: Theme.bg; border.color: Theme.keyLine
                            Text { anchors.centerIn: parent; text: page.sel ? page.sel.label : "?"; color: Theme.text
                                   font.pixelSize: 12; font.weight: Font.DemiBold } }
                ColumnLayout { spacing: 2; Layout.fillWidth: true
                    Text { text: page.sel ? "Key " + page.sel.label : "No key selected"; color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold }
                    Text { text: { page.rev; const d = page.sel ? KeyMap.describe(page.sel.keyIndex) : ""
                                   return page.sel ? (d ? "Assigned: " + d : "Default function") : "Click a key on the keyboard" }
                           color: Theme.dim; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                }
                AppButton { text: "Reset"; visible: page.sel !== null; enabled: { page.rev; return page.sel && KeyMap.describe(page.sel.keyIndex) !== "" }
                            onClicked: KeyMap.reset(page.sel.keyIndex) }
            }
        }
    }

    RowLayout {
        visible: page.tab === "key"; spacing: 6; Layout.fillWidth: true
        Text { text: "With"; color: Theme.faint; font.pixelSize: 11 }
        ModChip { id: mCtrl; text: "Ctrl" }
        ModChip { id: mShift; text: "Shift" }
        ModChip { id: mAlt; text: "Alt ⌥" }
        ModChip { id: mWin; text: "Win ⌘" }
    }
    ColumnLayout {
        visible: page.tab === "macro"; Layout.fillWidth: true; spacing: 10
        Text { visible: Macros.macros.length === 0; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim; font.pixelSize: 12
               text: "No macros yet — create one on the Macros page." }
        Text { text: "Play"; color: Theme.faint; font.pixelSize: 11; visible: Macros.macros.length > 0 }
        Segmented {
            visible: Macros.macros.length > 0; Layout.fillWidth: true
            model: [{label: "Once", value: 0}, {label: "Repeat", value: 1}, {label: "Toggle", value: 2}, {label: "Held", value: 3}]
            value: page.macroMode; onActivated: (v) => page.macroMode = v
        }
        RowLayout {
            visible: page.macroMode === 1 && Macros.macros.length > 0; spacing: 8
            Text { text: "Repeat"; color: Theme.dim; font.pixelSize: 12 }
            TextField { id: cnt; Layout.preferredWidth: 60; text: page.macroCount; color: Theme.text; font.pixelSize: 13; leftPadding: 8
                        validator: IntValidator { bottom: 1; top: 255 }
                        background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: cnt.activeFocus ? Theme.accent : Theme.line }
                        onTextEdited: page.macroCount = Math.max(1, Math.min(255, parseInt(text) || 1)) }
            Text { text: "times (1–255)"; color: Theme.faint; font.pixelSize: 11 }
        }
        Text { visible: Macros.macros.length > 0; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11
               text: "Once · Repeat N times · Toggle: runs until the key is pressed again · Held: runs while the key is held. Only “Repeat” is confirmed from the vendor app; the other three are inferred." }
        Repeater {
            model: Macros.macros
            delegate: Rectangle {
                Layout.fillWidth: true; height: 34; radius: 5
                color: mma.containsMouse && page.sel ? Theme.hover : Theme.raised
                Text { anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                       text: (index + 1) + "  " + modelData.name; font.pixelSize: 13; color: page.sel ? Theme.text : Theme.faint }
                Text { anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                       visible: mma.containsMouse && page.sel; text: "Assign"; color: Theme.accent; font.pixelSize: 11; font.weight: Font.DemiBold }
                MouseArea { id: mma; anchors.fill: parent; hoverEnabled: true
                            cursorShape: page.sel ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: page.assignMacro(index) }
            }
        }
    }
    TextField {
        id: search; visible: page.tab !== "macro"; Layout.fillWidth: true; placeholderText: "Search"; color: Theme.text
        placeholderTextColor: Theme.faint; font.pixelSize: 13; leftPadding: 12
        background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: search.activeFocus ? Theme.accent : Theme.line }
        onTextChanged: page.filter = text
    }
    Column {
        Layout.fillWidth: true; spacing: 2
        Repeater {
            model: page.items
            delegate: Rectangle {
                width: parent.width; height: 32; radius: 5
                color: ima.containsMouse && page.sel ? Theme.hover : "transparent"
                Text { anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                       text: modelData.name; font.pixelSize: 13; color: page.sel ? Theme.text : Theme.faint }
                Text { anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                       visible: ima.containsMouse && page.sel; text: "Assign"; color: Theme.accent; font.pixelSize: 11; font.weight: Font.DemiBold }
                MouseArea { id: ima; anchors.fill: parent; hoverEnabled: true
                            cursorShape: page.sel ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: page.assign(modelData) }
            }
        }
    }

    footer: [
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            AppButton { text: "Reset layer"; danger: true; onClicked: KeyMap.resetLayer() }
            Item { Layout.fillWidth: true }
            Text { text: Object.keys(KeyMap.labels).length + " changed"; color: Theme.dim; font.pixelSize: 12 }
            AppButton { text: "Upload layer"; primary: true; enabled: Device.ready && Device.writesEnabled
                        onClicked: KeyMap.apply() }
        },
        WriteGate { Layout.fillWidth: true }
    ]
}
