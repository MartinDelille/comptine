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
    property bool acceptCloses: true
    property alias okEnabled: acceptButton.enabled
    signal acceptRequested

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent

    // Override this property to control when Enter key can submit
    property bool canSubmit: true

    // Submit on Enter key (only if canSubmit is true)
    Shortcut {
        sequence: "Return"
        enabled: root.visible && root.okEnabled && root.canSubmit
        onActivated: {
            if (root.acceptCloses)
                root.accept();
            else
                root.acceptRequested();
        }
    }

    footer: DialogButtonBox {
        spacing: 10

        Button {
            id: acceptButton

            focus: true
            text: root.acceptButtonText
            DialogButtonBox.buttonRole: root.acceptCloses ? DialogButtonBox.AcceptRole : DialogButtonBox.ActionRole
            onClicked: {
                if (!root.acceptCloses)
                    root.acceptRequested();
            }
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
