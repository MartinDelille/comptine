import QtQuick
import QtQuick.Controls

// Base dialog with common functionality:
// - Enter key submits (if valid)
// - Escape key cancels
// - Consistent modal behavior
Dialog {
    id: root

    property string acceptButtonText: qsTr("Ok")
    property string discardButtonText: qsTr("")
    property string rejectButtonText: qsTr("Cancel")
    property alias okEnabled: acceptButton.enabled

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent

    // Override this property to control when Enter key can submit
    property bool canSubmit: true

    // Helper function to commit pending edits and accept
    function commitAndAccept() {
        // Force any focused text field to commit its value
        // by moving focus away before accepting
        if (activeFocusItem) {
            activeFocusItem.focus = false;
        }
        root.accept();
    }

    // Submit on Enter key (only if canSubmit is true)
    Shortcut {
        sequence: "Return"
        enabled: root.visible && root.okEnabled && root.canSubmit
        onActivated: root.commitAndAccept()
    }

    footer: DialogButtonBox {
        spacing: 10

        Button {
            id: acceptButton

            focus: true
            text: root.acceptButtonText
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }

        Button {
            id: discardButton
            text: root.discardButtonText
            visible: text.length > 0
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
        }

        Button {
            id: rejectButton
            text: root.rejectButtonText
            visible: text.length > 0
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
