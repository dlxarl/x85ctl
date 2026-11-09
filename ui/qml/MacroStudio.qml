import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Macros: a visual timeline of key / mouse / wait steps on the stage, recorded from the real keyboard
// or added by hand. Stored in the profile, uploaded with cmd 0x15, bound to keys on Assignments → Macro.
Workspace {
    id: page
    title: "Macros"
    hint: Recorder.recording ? "Recording — type on your keyboard. Keys go into the macro, not into the app."
                             : cur >= 0 ? "Click a step to edit it · new steps go after the selected one"
                                        : "Create a macro, then record it from your keyboard"

    property int cur: -1
    property int selStep: -1
    property string kind: "key"            // key | mouse | wait
    property string keyMode: "key"         // key (press & release) | keydown | keyup
    property int mouseBtn: 1
    readonly property bool canWrite: Device.ready && Device.writesEnabled
    readonly property var steps: { Macros.macros; return cur >= 0 ? Macros.steps(cur) : [] }
    readonly property int insertAt: selStep >= 0 ? selStep + 1 : -1
    readonly property var sel: selStep >= 0 && selStep < steps.length ? steps[selStep] : null
    Component.onCompleted: if (Macros.macros.length > 0) cur = 0
    onCurChanged: { selStep = -1; if (Recorder.recording) Recorder.stop() }
    Connections {
        target: Macros
        function onChanged() {
            if (page.cur >= Macros.macros.length) page.cur = Macros.macros.length - 1
            if (page.selStep >= page.steps.length) page.selStep = page.steps.length - 1
        }
    }
    function addStep(type, value, mods) {
        const n = Macros.addStep(cur, type, value, mods || 0, insertAt)
        if (selStep >= 0) selStep += n
    }
    function keyIndexOf(usage) {
        const t = KeyMap.keyTargets
        for (let i = 0; i < t.length; ++i) if (t[i].usage === usage) return i
        return 0
    }
    function mouseName(v) { return v === 1 ? "Left" : v === 2 ? "Right" : v === 4 ? "Middle" : "Button" }
    function keyName(label) { return label.replace(/^(Press|Release) /, "") }

    // ======================= stage: the timeline =======================
    stage: Item {
        anchors.fill: parent

        Text {
            visible: page.cur < 0
            anchors.centerIn: parent
            text: Macros.macros.length ? "Choose a macro on the left" : "No macros yet — press “New macro” on the left"
            color: Theme.dim; font.pixelSize: 15
        }

        ColumnLayout {
            visible: page.cur >= 0
            anchors.fill: parent
            spacing: 16

            // ---- header: name, length, record ----
            RowLayout {
                Layout.fillWidth: true; spacing: 14
                ColumnLayout {
                    spacing: 2; Layout.fillWidth: true
                    Text { text: page.cur >= 0 && Macros.macros[page.cur] ? Macros.macros[page.cur].name : ""
                           color: Theme.text; font.pixelSize: 22; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
                    Text { text: { Macros.macros; const ms = page.cur >= 0 ? Macros.totalMs(page.cur) : 0
                                   return page.steps.length + " steps · " + (ms >= 1000 ? (ms / 1000).toFixed(2) + " s" : ms + " ms") }
                           color: Theme.dim; font.pixelSize: 12 }
                }
                AppSwitch { text: "Record delays"; checked: Recorder.recordDelays; enabled: !Recorder.recording
                            onToggled: Recorder.recordDelays = checked }
                Rectangle {   // record / stop
                    id: rec
                    implicitWidth: recRow.implicitWidth + 32; height: 40; radius: 20
                    color: Recorder.recording ? Theme.danger : recMa.containsMouse ? Qt.lighter(Theme.raised, 1.3) : Theme.raised
                    border.color: Recorder.recording ? "transparent" : Qt.alpha(Theme.danger, 0.6)
                    Row {
                        id: recRow; anchors.centerIn: parent; spacing: 9
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 12; height: 12; radius: Recorder.recording ? 2 : 6
                            color: Recorder.recording ? "white" : Theme.danger
                            SequentialAnimation on opacity {
                                running: Recorder.recording; loops: Animation.Infinite
                                NumberAnimation { to: 0.35; duration: 500 } NumberAnimation { to: 1; duration: 500 }
                                onRunningChanged: if (!running) parent.opacity = 1
                            }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 13; font.weight: Font.DemiBold
                               color: Recorder.recording ? "white" : Theme.text
                               text: Recorder.recording ? "Stop  (" + Recorder.recorded + ")" : page.selStep >= 0 ? "Record here" : "Record" }
                    }
                    MouseArea { id: recMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: Recorder.recording ? Recorder.stop() : Recorder.start(page.cur, page.insertAt) }
                }
            }

            // ---- timeline ----
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                radius: 10; color: Qt.alpha(Theme.surface, 0.7)
                border.color: Recorder.recording ? Qt.alpha(Theme.danger, 0.7) : Theme.line

                Text {
                    visible: page.steps.length === 0
                    anchors.centerIn: parent; horizontalAlignment: Text.AlignHCenter
                    text: Recorder.recording ? "Listening… press keys on your keyboard"
                                             : "Empty macro\nPress Record and type, or add steps on the left"
                    color: Recorder.recording ? Theme.danger : Theme.faint; font.pixelSize: 14; lineHeight: 1.3
                }

                Flickable {
                    id: flick
                    anchors.fill: parent; anchors.margins: 16
                    contentHeight: flow.implicitHeight; clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {}
                    onContentHeightChanged: if (Recorder.recording) contentY = Math.max(0, contentHeight - height)
                    MouseArea { width: flick.width; height: Math.max(flick.height, flow.implicitHeight); onClicked: page.selStep = -1 }

                    Flow {
                        id: flow
                        width: flick.width; spacing: 8
                        Repeater {
                            model: page.steps
                            delegate: Item {
                                id: chip
                                readonly property bool on: index === page.selStep
                                readonly property int t: modelData.type      // 0 delay, 1 key down, 2 key up, 3 mouse down, 4 mouse up
                                readonly property bool isDelay: t === 0
                                readonly property bool isDown: t === 1 || t === 3
                                readonly property color tint: isDelay ? Theme.warn : (t >= 3 ? "#b18cff" : Theme.accent)
                                width: body.width + (insertMark.visible ? 10 : 0); height: 40

                                Rectangle {   // where new steps / a new recording will go
                                    id: insertMark
                                    visible: chip.on
                                    x: body.width + 4; y: 4; width: 2; height: 32; radius: 1; color: Theme.text
                                    SequentialAnimation on opacity { running: insertMark.visible; loops: Animation.Infinite
                                        NumberAnimation { to: 0.2; duration: 500 } NumberAnimation { to: 1; duration: 500 } }
                                }
                                Rectangle {
                                    id: body
                                    height: 40; radius: chip.isDelay ? 20 : 8
                                    width: row.implicitWidth + (chip.isDelay ? 26 : 22)
                                    color: chip.on ? Qt.alpha(chip.tint, 0.28)
                                         : cma.containsMouse ? Qt.alpha(chip.tint, 0.16)
                                         : chip.isDelay ? Qt.alpha(Theme.warn, 0.07)
                                         : chip.isDown ? Qt.alpha(chip.tint, 0.12) : Theme.raised
                                    border.color: chip.on ? chip.tint : chip.isDown || chip.isDelay ? Qt.alpha(chip.tint, 0.45) : Theme.line
                                    border.width: chip.on ? 2 : 1
                                    Row {
                                        id: row; anchors.centerIn: parent; spacing: 6
                                        Icon {
                                            anchors.verticalCenter: parent.verticalCenter; size: 14
                                            name: chip.isDelay ? "clock" : chip.t >= 3 ? "mouse" : chip.isDown ? "down" : "up"
                                            color: chip.isDown || chip.isDelay ? chip.tint : Theme.dim
                                        }
                                        Icon {
                                            visible: chip.t >= 3; anchors.verticalCenter: parent.verticalCenter; size: 12
                                            name: chip.isDown ? "down" : "up"; color: chip.isDown ? chip.tint : Theme.dim
                                        }
                                        Text {
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: chip.isDelay ? modelData.value + " ms"
                                                : chip.t >= 3 ? page.mouseName(modelData.value)
                                                : page.keyName(modelData.label)
                                            font.pixelSize: chip.isDelay ? 12 : 13
                                            font.weight: chip.isDown ? Font.DemiBold : Font.Normal
                                            color: chip.isDelay ? Theme.warn : chip.isDown ? Theme.text : Theme.dim
                                        }
                                    }
                                    MouseArea { id: cma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                                onClicked: page.selStep = chip.on ? -1 : index }
                                }
                            }
                        }
                    }
                }
            }

            // ---- legend ----
            Row {
                spacing: 18
                Repeater {
                    model: [{i: "down", t: "Key pressed", c: Theme.accent}, {i: "up", t: "Key released", c: Theme.dim},
                            {i: "mouse", t: "Mouse button", c: "#b18cff"}, {i: "clock", t: "Wait", c: Theme.warn}]
                    delegate: Row { spacing: 6
                        Icon { name: modelData.i; size: 13; color: modelData.c; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: modelData.t; color: Theme.faint; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                }
            }
        }
    }

    // ======================= panel =======================
    SectionLabel { text: "Your macros" }
    Column {
        Layout.fillWidth: true; spacing: 4
        Repeater {
            model: Macros.macros
            delegate: Rectangle {
                readonly property bool on: index === page.cur
                width: parent.width; height: 40; radius: Theme.radius
                color: on ? Qt.alpha(Theme.accent, 0.14) : rma.containsMouse ? Theme.hover : Theme.raised
                border.color: on ? Theme.accent : "transparent"
                MouseArea { id: rma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: page.cur = index }
                RowLayout {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 6 } spacing: 6
                    Text { text: (index + 1) + "  " + modelData.name; color: Theme.text; font.pixelSize: 13; elide: Text.ElideRight
                           font.weight: on ? Font.DemiBold : Font.Normal; Layout.fillWidth: true; Layout.minimumWidth: 0 }
                    Text { text: modelData.steps + " steps"; color: Theme.faint; font.pixelSize: 11 }
                    IconButton { glyph: "✕"; tip: "Delete macro"; onClicked: Macros.remove(index) }
                }
            }
        }
    }
    AppButton { text: "New macro"; Layout.fillWidth: true; enabled: Macros.macros.length < 100 && !Recorder.recording
                onClicked: page.cur = Macros.add("") }

    SectionLabel { visible: page.cur >= 0; text: "Name" }
    TextField {
        id: nameField
        visible: page.cur >= 0; Layout.fillWidth: true; color: Theme.text; font.pixelSize: 13; leftPadding: 10
        text: page.cur >= 0 && Macros.macros[page.cur] ? Macros.macros[page.cur].name : ""
        background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: nameField.activeFocus ? Theme.accent : Theme.line }
        onEditingFinished: Macros.rename(page.cur, text)
    }

    // ---- selected step ----
    SectionLabel { visible: page.sel !== null; text: "Step " + (page.selStep + 1) }
    ColumnLayout {
        visible: page.sel !== null; Layout.fillWidth: true; spacing: 8
        RowLayout {   // delay value
            visible: page.sel !== null && page.sel.type === 0; spacing: 8; Layout.fillWidth: true
            Text { text: "Wait"; color: Theme.dim; font.pixelSize: 12 }
            TextField {
                id: delayField; Layout.fillWidth: true; color: Theme.text; font.pixelSize: 13; leftPadding: 10
                text: page.sel && page.sel.type === 0 ? page.sel.value : ""
                validator: IntValidator { bottom: 10; top: 65535 }
                background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: delayField.activeFocus ? Theme.accent : Theme.line }
                onEditingFinished: Macros.setStepValue(page.cur, page.selStep, parseInt(text) || 10)
            }
            Text { text: "ms"; color: Theme.dim; font.pixelSize: 12 }
        }
        AppCombo {   // key of a key step
            visible: page.sel !== null && (page.sel.type === 1 || page.sel.type === 2); Layout.fillWidth: true
            model: KeyMap.keyTargets; textRole: "name"
            currentIndex: page.sel && (page.sel.type === 1 || page.sel.type === 2) ? page.keyIndexOf(page.sel.value) : 0
            onActivated: (i) => Macros.setStepValue(page.cur, page.selStep, KeyMap.keyTargets[i].usage)
        }
        Segmented {   // button of a mouse step
            visible: page.sel !== null && page.sel.type >= 3; Layout.fillWidth: true
            model: [{label: "Left", value: 1}, {label: "Middle", value: 4}, {label: "Right", value: 2}]
            value: page.sel ? page.sel.value : 1
            onActivated: (v) => Macros.setStepValue(page.cur, page.selStep, v)
        }
        RowLayout {
            spacing: 6; Layout.fillWidth: true
            AppButton { text: "← Move"; Layout.fillWidth: true; enabled: page.selStep > 0
                        onClicked: { Macros.moveStep(page.cur, page.selStep, -1); page.selStep-- } }
            AppButton { text: "Move →"; Layout.fillWidth: true; enabled: page.selStep < page.steps.length - 1
                        onClicked: { Macros.moveStep(page.cur, page.selStep, 1); page.selStep++ } }
            AppButton { text: "Delete"; danger: true; Layout.fillWidth: true
                        onClicked: { Macros.removeStep(page.cur, page.selStep); page.selStep = Math.min(page.selStep, page.steps.length - 2) } }
        }
    }

    // ---- add by hand ----
    SectionLabel { visible: page.cur >= 0; text: page.selStep >= 0 ? "Add after step " + (page.selStep + 1) : "Add at the end" }
    Segmented {
        visible: page.cur >= 0; Layout.fillWidth: true
        model: [{label: "Key", value: "key"}, {label: "Mouse", value: "mouse"}, {label: "Wait", value: "wait"}]
        value: page.kind; onActivated: (v) => page.kind = v
    }
    ColumnLayout {
        visible: page.cur >= 0 && page.kind === "key"; Layout.fillWidth: true; spacing: 8
        Segmented {
            Layout.fillWidth: true
            model: [{label: "Press + release", value: "key"}, {label: "Press", value: "keydown"}, {label: "Release", value: "keyup"}]
            value: page.keyMode; onActivated: (v) => page.keyMode = v
        }
        AppCombo { id: keyBox; Layout.fillWidth: true; model: KeyMap.keyTargets; textRole: "name"; currentIndex: 0 }
        RowLayout {
            spacing: 6; visible: page.keyMode !== "keyup"
            Text { text: "With"; color: Theme.faint; font.pixelSize: 11 }
            ModChip { id: mc; text: "Ctrl" }
            ModChip { id: ms; text: "Shift" }
            ModChip { id: ma2; text: "Alt ⌥" }
            ModChip { id: mw; text: "Win ⌘" }
        }
        AppButton { text: "Add key"; Layout.fillWidth: true
                    onClicked: page.addStep(page.keyMode, KeyMap.keyTargets[keyBox.currentIndex].usage,
                                            page.keyMode === "keyup" ? 0 : ((mc.checked ? 1 : 0) | (ms.checked ? 2 : 0) | (ma2.checked ? 4 : 0) | (mw.checked ? 8 : 0))) }
    }
    ColumnLayout {
        visible: page.cur >= 0 && page.kind === "mouse"; Layout.fillWidth: true; spacing: 8
        Segmented {
            Layout.fillWidth: true
            model: [{label: "Left", value: 1}, {label: "Middle", value: 4}, {label: "Right", value: 2}]
            value: page.mouseBtn; onActivated: (v) => page.mouseBtn = v
        }
        RowLayout {
            spacing: 6
            AppButton { text: "Click"; Layout.fillWidth: true; onClicked: page.addStep("mouse", page.mouseBtn) }
            AppButton { text: "Press"; Layout.fillWidth: true; onClicked: page.addStep("mousedown", page.mouseBtn) }
            AppButton { text: "Release"; Layout.fillWidth: true; onClicked: page.addStep("mouseup", page.mouseBtn) }
        }
    }
    RowLayout {
        visible: page.cur >= 0 && page.kind === "wait"; Layout.fillWidth: true; spacing: 8
        TextField {
            id: waitMs; Layout.fillWidth: true; text: "50"; color: Theme.text; font.pixelSize: 13; leftPadding: 10
            validator: IntValidator { bottom: 10; top: 65535 }
            background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: waitMs.activeFocus ? Theme.accent : Theme.line }
        }
        Text { text: "ms"; color: Theme.dim; font.pixelSize: 12 }
        AppButton { text: "Add wait"; onClicked: page.addStep("delay", parseInt(waitMs.text) || 50) }
    }

    // ---- whole macro ----
    SectionLabel { visible: page.cur >= 0 && page.steps.length > 0; text: "All steps" }
    RowLayout {
        visible: page.cur >= 0 && page.steps.length > 0; Layout.fillWidth: true; spacing: 6
        TextField {
            id: allMs; Layout.preferredWidth: 64; text: "30"; color: Theme.text; font.pixelSize: 13; leftPadding: 8
            validator: IntValidator { bottom: 10; top: 65535 }
            background: Rectangle { radius: Theme.radius; color: Theme.bg; border.color: allMs.activeFocus ? Theme.accent : Theme.line }
        }
        AppButton { text: "Set all waits"; Layout.fillWidth: true; onClicked: Macros.setAllDelays(page.cur, parseInt(allMs.text) || 30) }
        AppButton { text: "No waits"; Layout.fillWidth: true; onClicked: Macros.setAllDelays(page.cur, 0) }
    }
    AppButton { visible: page.cur >= 0 && page.steps.length > 0; text: "Clear all steps"; danger: true; Layout.fillWidth: true
                onClicked: { Macros.clearSteps(page.cur); page.selStep = -1 } }

    SectionLabel { text: "Keyboard memory" }
    Rectangle {
        Layout.fillWidth: true; height: 6; radius: 3; color: Theme.raised
        Rectangle { width: parent.width * Math.min(1, Macros.bytesUsed / Macros.bytesMax); height: parent.height; radius: 3
                    color: Macros.error !== "" ? Theme.danger : Theme.accent }
    }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; font.pixelSize: 11
           color: Macros.error !== "" ? Theme.danger : Theme.faint
           text: Macros.error !== "" ? Macros.error : Macros.bytesUsed + " of " + Macros.bytesMax + " bytes used (each step takes 4)." }
    Text { Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.faint; font.pixelSize: 11
           text: "Recording captures keys by position, so it works with any layout. Waits shorter than 10 ms are not recorded. The macro format is decoded from the vendor app and not yet tried on a real keyboard." }

    footer: [
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            Text { text: Macros.macros.length + " macros"; color: Theme.dim; font.pixelSize: 12 }
            Item { Layout.fillWidth: true }
            AppButton { text: "Upload macros"; primary: true
                        enabled: page.canWrite && Macros.macros.length > 0 && Macros.error === "" && !Recorder.recording
                        onClicked: Macros.apply() }
        },
        WriteGate { Layout.fillWidth: true }
    ]
}
