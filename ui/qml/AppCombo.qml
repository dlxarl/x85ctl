import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: c
    implicitHeight: 32
    font.pixelSize: Theme.fontBody
    background: Rectangle { radius: 7; color: Theme.surface; border.color: c.activeFocus || c.popup.visible ? Theme.accent : Theme.line }
    contentItem: Text { leftPadding: 10; rightPadding: 24; text: c.displayText; font: c.font; color: Theme.text
                        verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
    indicator: Text { x: c.width - width - 10; y: (c.height - height) / 2; text: "⌄"; color: Theme.dim; font.pixelSize: 14 }
    delegate: ItemDelegate {
        width: c.width; height: 28
        // textAt() works for every model kind (JS arrays, QVariantList, list models); the old
        // Array.isArray() check failed for QVariantList, so lists from C++ showed empty rows.
        contentItem: Text { text: c.textAt(index)
                            color: c.currentIndex === index ? Theme.accent : Theme.text; font.pixelSize: Theme.fontBody
                            verticalAlignment: Text.AlignVCenter }
        background: Rectangle { color: highlighted ? Theme.raised : "transparent" }
        highlighted: c.highlightedIndex === index
    }
    popup: Popup {
        y: c.height + 4; width: c.width; implicitHeight: Math.min(contentItem.implicitHeight + 8, 320); padding: 4
        contentItem: ListView { clip: true; implicitHeight: contentHeight; model: c.popup.visible ? c.delegateModel : null
                                currentIndex: c.highlightedIndex; ScrollIndicator.vertical: ScrollIndicator {} }
        background: Rectangle { radius: 8; color: Theme.surface; border.color: Theme.line }
    }
}
