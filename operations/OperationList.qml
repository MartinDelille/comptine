pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

ListView {
    id: root

    required property var account

    model: account
    currentIndex: account?.currentOperationIndex ?? -1
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
        account.previousOperation(event.modifiers & Qt.ShiftModifier);
    }

    Keys.onDownPressed: event => {
        account.nextOperation(event.modifiers & Qt.ShiftModifier);
    }

    delegate: OperationDelegate {
        required property int index
        required property var model
        width: root.width - scrollBar.width
        operation: model.operation
        balance: model.balance
        selected: model.selected
        focused: root.currentIndex === index
        alternate: index % 2 === 0

        MouseArea {
            anchors.fill: parent
            onClicked: mouse => {
                if (mouse.modifiers & Qt.ControlModifier) {
                    // Cmd/Ctrl+click: toggle selection
                    root.account.toggleSelectionAt(parent.index);
                } else {
                    root.account.selectAt(parent.index, mouse.modifiers & Qt.ShiftModifier);
                }
            }
        }
    }
}
