pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

ListView {
    id: root

    signal previousOperation(bool shift)
    signal nextOperation(bool shift)
    signal toggleSelectionAt(int index)
    signal selectAt(int index, bool extend)

    activeFocusOnTab: true
    clip: true
    focus: true
    keyNavigationEnabled: false  // We handle key navigation ourselves
    highlightFollowsCurrentItem: false  // Don't auto-scroll highlight

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
    }
    onCurrentIndexChanged: {
        root.positionViewAtIndex(currentIndex, ListView.Contain);
    }

    Keys.onUpPressed: event => {
        previousOperation(event.modifiers & Qt.ShiftModifier);
    }

    Keys.onDownPressed: event => {
        nextOperation(event.modifiers & Qt.ShiftModifier);
    }

    delegate: OperationDelegate {
        required property int index
        required property var model
        width: root.width - scrollBar.width
        operation: model.operation
        balance: model.balance
        selected: model.selected
        focused: root.activeFocus && root.currentIndex === index
        alternate: index % 2 === 0

        MouseArea {
            anchors.fill: parent
            onClicked: mouse => {
                root.forceActiveFocus();

                if (mouse.modifiers & Qt.ControlModifier) {
                    // Cmd/Ctrl+click: toggle selection
                    root.toggleSelectionAt(parent.index);
                } else if (mouse.modifiers & Qt.ShiftModifier) {
                    // Shift+click: range selection from current operation
                    root.selectAt(parent.index, true);
                } else {
                    // Plain click: single selection (clear others)
                    root.selectAt(parent.index, false);
                }

                // Update cursor (current operation) after selection handling
                root.currentIndex = parent.index;
            }
        }
    }
}
