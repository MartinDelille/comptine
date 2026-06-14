pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

FocusScope {
    id: root

    required property var budgetData
    required property var navigation

    activeFocusOnTab: true  // Allow Tab to focus this component

    // Convenience property to access current account
    readonly property var currentAccount: navigation.currentAccount

    onActiveFocusChanged: {
        if (activeFocus) {
            listView.forceActiveFocus();
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: root.budgetData.operationModel
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        focus: true
        keyNavigationEnabled: false  // We handle key navigation ourselves
        highlightFollowsCurrentItem: false  // Don't auto-scroll highlight
        currentIndex: root.currentAccount ? root.currentAccount.currentOperationIndex : -1

        Connections {
            target: root.navigation
            function onOperationSelected(index) {
                // Navigate from CategoryDetailView: focus and scroll to the operation
                if (root.currentAccount) {
                    root.currentAccount.currentOperationIndex = index;
                    root.currentAccount.selectAt(index, false);
                }
                listView.positionViewAtIndex(index, ListView.Center);
            }
            function onCurrentAccountChanged() {
                // When account changes, scroll to the account's current operation
                // Selection is already per-account, no need to call select()
                if (!root.currentAccount)
                    return;
                let idx = root.currentAccount.currentOperationIndex;
                if (idx < 0 && listView.count > 0) {
                    // Account has no current operation yet, default to first operation
                    idx = 0;
                    root.currentAccount.currentOperationIndex = idx;
                    root.currentAccount.selectAt(idx, false);
                }
                if (idx >= 0 && idx < listView.count) {
                    listView.positionViewAtIndex(idx, ListView.Contain);
                }
            }
        }

        Connections {
            target: root.budgetData.operationModel
            function onOperationFocused(index) {
                // Programmatic selection (e.g., after undo/redo): scroll to the operation
                listView.positionViewAtIndex(index, ListView.Center);
            }
        }

        Keys.onUpPressed: event => {
            root.navigation.previousOperation(event.modifiers & Qt.ShiftModifier);
            if (root.currentAccount) {
                positionViewAtIndex(root.currentAccount.currentOperationIndex, ListView.Contain);
            }
        }

        Keys.onDownPressed: event => {
            root.navigation.nextOperation(event.modifiers & Qt.ShiftModifier);
            if (root.currentAccount) {
                positionViewAtIndex(root.currentAccount.currentOperationIndex, ListView.Contain);
            }
        }

        // Cmd+A to select all
        Keys.onPressed: event => {
            if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_A) {
                if (root.currentAccount) {
                    root.currentAccount.selectRange(0, count - 1);
                }
                event.accepted = true;
            }
        }

        delegate: OperationDelegate {
            required property int index
            required property var model
            width: listView.width - scrollBar.width
            operation: model.operation
            balance: model.balance
            selected: model.selected
            focused: listView.activeFocus && listView.currentIndex === index
            alternate: index % 2 === 0

            MouseArea {
                anchors.fill: parent
                onClicked: mouse => {
                    listView.forceActiveFocus();

                    if (mouse.modifiers & Qt.ControlModifier) {
                        // Cmd/Ctrl+click: toggle selection
                        if (root.currentAccount) {
                            root.currentAccount.toggleSelectionAt(parent.index);
                        }
                    } else if (mouse.modifiers & Qt.ShiftModifier) {
                        // Shift+click: range selection from current operation
                        if (root.currentAccount) {
                            root.currentAccount.selectAt(parent.index, true);
                        }
                    } else {
                        // Plain click: single selection (clear others)
                        if (root.currentAccount) {
                            root.currentAccount.selectAt(parent.index, false);
                        }
                    }

                    // Update cursor (current operation) after selection handling
                    if (root.currentAccount) {
                        root.currentAccount.currentOperationIndex = parent.index;
                    }
                }
            }
        }

        ScrollBar.vertical: ScrollBar {
            id: scrollBar
        }
    }

    // Expose properties for parent access
    property alias count: listView.count
    property alias currentIndex: listView.currentIndex
}
