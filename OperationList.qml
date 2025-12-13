import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: focusScope
    activeFocusOnTab: true  // Allow Tab to focus this component

    // Convenience property to access current account
    readonly property var currentAccount: AppState.navigation.currentAccount

    onActiveFocusChanged: {
        if (activeFocus) {
            listView.forceActiveFocus();
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: AppState.data.operationModel
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        focus: true
        keyNavigationEnabled: false  // We handle key navigation ourselves
        highlightFollowsCurrentItem: false  // Don't auto-scroll highlight
        currentIndex: currentAccount ? currentAccount.currentOperationIndex : -1

        // Restore focus when YAML file is loaded
        Connections {
            target: AppState.file
            function onYamlFileLoaded() {
                if (listView.count > 0 && currentAccount) {
                    // Use the index from the file (already set in account.currentOperationIndex)
                    let idx = Math.min(currentAccount.currentOperationIndex, listView.count - 1);
                    if (idx < 0)
                        idx = 0;
                    currentAccount.currentOperationIndex = idx;
                    currentAccount.selectAt(idx, false);
                    listView.positionViewAtIndex(idx, ListView.Center);
                }
                listView.forceActiveFocus();
            }
            function onDataLoaded() {
                // For CSV import: sync ListView currentIndex with model selection
                if (listView.count > 0 && currentAccount && currentAccount.currentOperationIndex < 0) {
                    currentAccount.currentOperationIndex = 0;
                    currentAccount.selectAt(0, false);
                }
                let idx = currentAccount ? currentAccount.currentOperationIndex : 0;
                listView.positionViewAtIndex(idx >= 0 ? idx : 0, ListView.Contain);
                listView.forceActiveFocus();
            }
        }

        Connections {
            target: AppState.navigation
            function onOperationSelected(index) {
                // Navigate from CategoryDetailView: focus and scroll to the operation
                if (currentAccount) {
                    currentAccount.currentOperationIndex = index;
                    currentAccount.selectAt(index, false);
                }
                listView.positionViewAtIndex(index, ListView.Center);
                listView.forceActiveFocus();
            }
            function onCurrentAccountChanged() {
                // When account changes, scroll to the account's current operation
                // Selection is already per-account, no need to call select()
                if (!currentAccount)
                    return;
                let idx = currentAccount.currentOperationIndex;
                if (idx < 0 && listView.count > 0) {
                    // Account has no current operation yet, default to first operation
                    idx = 0;
                    currentAccount.currentOperationIndex = idx;
                    currentAccount.selectAt(idx, false);
                }
                if (idx >= 0 && idx < listView.count) {
                    listView.positionViewAtIndex(idx, ListView.Contain);
                }
            }
        }

        Connections {
            target: AppState.data.operationModel
            function onOperationFocused(index) {
                // Programmatic selection (e.g., after undo/redo): scroll to the operation
                listView.positionViewAtIndex(index, ListView.Center);
            }
        }

        Keys.onUpPressed: event => {
            AppState.navigation.previousOperation(event.modifiers & Qt.ShiftModifier);
            if (currentAccount) {
                positionViewAtIndex(currentAccount.currentOperationIndex, ListView.Contain);
            }
        }

        Keys.onDownPressed: event => {
            AppState.navigation.nextOperation(event.modifiers & Qt.ShiftModifier);
            if (currentAccount) {
                positionViewAtIndex(currentAccount.currentOperationIndex, ListView.Contain);
            }
        }

        // Cmd+A to select all
        Keys.onPressed: event => {
            if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_A) {
                if (currentAccount) {
                    currentAccount.selectRange(0, count - 1);
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
                        if (currentAccount) {
                            currentAccount.toggleSelectionAt(parent.index);
                        }
                    } else if (mouse.modifiers & Qt.ShiftModifier) {
                        // Shift+click: range selection from current operation
                        if (currentAccount) {
                            currentAccount.selectAt(parent.index, true);
                        }
                    } else {
                        // Plain click: single selection (clear others)
                        if (currentAccount) {
                            currentAccount.selectAt(parent.index, false);
                        }
                    }

                    // Update cursor (current operation) after selection handling
                    if (currentAccount) {
                        currentAccount.currentOperationIndex = parent.index;
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
