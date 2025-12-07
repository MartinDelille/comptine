import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: focusScope
    activeFocusOnTab: true  // Allow Tab to focus this component

    onActiveFocusChanged: {
        if (activeFocus) {
            listView.forceActiveFocus();
        }
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: budgetData.operationModel

        clip: true
        boundsBehavior: Flickable.StopAtBounds
        focus: true
        keyNavigationEnabled: false  // We handle key navigation ourselves
        highlightFollowsCurrentItem: false  // Don't auto-scroll highlight
        currentIndex: budgetData.currentOperationIndex

        // Sync currentIndex back to budgetData
        onCurrentIndexChanged: {
            if (currentIndex >= 0) {
                budgetData.currentOperationIndex = currentIndex;
            }
        }

        // Restore focus when YAML file is loaded (not after CSV import, which handles its own selection)
        Connections {
            target: budgetData
            function onYamlFileLoaded() {
                if (listView.count > 0) {
                    // Use the index from the file (already set in budgetData.currentOperationIndex)
                    let idx = Math.min(budgetData.currentOperationIndex, listView.count - 1);
                    if (idx < 0)
                        idx = 0;
                    listView.currentIndex = idx;
                    budgetData.operationModel.select(idx, false);
                    listView.positionViewAtIndex(idx, ListView.Contain);
                }
                listView.forceActiveFocus();
            }
            function onDataLoaded() {
                // For CSV import: sync ListView currentIndex with model selection
                // The model already has the correct selection, just update the view
                if (listView.count > 0 && listView.currentIndex < 0) {
                    listView.currentIndex = 0;
                }
                listView.positionViewAtIndex(listView.currentIndex >= 0 ? listView.currentIndex : 0, ListView.Contain);
                listView.forceActiveFocus();
            }
            function onOperationSelected(index) {
                // Navigate from CategoryDetailView: focus and scroll to the operation
                listView.currentIndex = index;
                listView.positionViewAtIndex(index, ListView.Center);
                listView.forceActiveFocus();
            }
        }

        Keys.onUpPressed: event => {
            if (currentIndex > 0) {
                currentIndex--;
                if (event.modifiers & Qt.ShiftModifier) {
                    // Shift+Up: extend selection
                    budgetData.operationModel.select(currentIndex, true);
                } else {
                    // Plain Up: single selection
                    budgetData.operationModel.select(currentIndex, false);
                }
                positionViewAtIndex(currentIndex, ListView.Contain);
            }
        }

        Keys.onDownPressed: event => {
            if (currentIndex < count - 1) {
                currentIndex++;
                if (event.modifiers & Qt.ShiftModifier) {
                    // Shift+Down: extend selection
                    budgetData.operationModel.select(currentIndex, true);
                } else {
                    // Plain Down: single selection
                    budgetData.operationModel.select(currentIndex, false);
                }
                positionViewAtIndex(currentIndex, ListView.Contain);
            }
        }

        // Cmd+A to select all
        Keys.onPressed: event => {
            if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_A) {
                budgetData.operationModel.selectRange(0, count - 1);
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
                    listView.currentIndex = parent.index;
                    listView.forceActiveFocus();

                    if (mouse.modifiers & Qt.ControlModifier) {
                        // Cmd/Ctrl+click: toggle selection
                        budgetData.operationModel.toggleSelection(parent.index);
                    } else if (mouse.modifiers & Qt.ShiftModifier) {
                        // Shift+click: range selection from current
                        budgetData.operationModel.selectRange(listView.currentIndex, parent.index);
                    } else {
                        // Plain click: single selection (clear others)
                        budgetData.operationModel.select(parent.index, false);
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
