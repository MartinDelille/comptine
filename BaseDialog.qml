import QtQuick
import QtQuick.Controls

// Base dialog with common functionality:
// - Enter key submits (if valid)
// - Escape key cancels
// - Consistent modal behavior
Dialog {
    id: root

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
        enabled: root.visible && root.canSubmit
        onActivated: root.commitAndAccept()
    }
}
