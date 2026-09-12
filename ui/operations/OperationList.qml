pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import model
import ui.common

ListView {
    id: root

    required property var account
    property string query: ""

    readonly property int visibleOperationCount: filterModel.count

    model: filterModel
    currentIndex: filterModel.currentOperationIndex
    activeFocusOnTab: true
    clip: true
    focus: true
    keyNavigationEnabled: false  // We handle key navigation ourselves
    highlightFollowsCurrentItem: false  // Don't auto-scroll highlight

    OperationFilterModel {
        id: filterModel
        account: root.account
        query: root.query
    }

    Label {
        anchors.centerIn: parent
        visible: root.account && root.account.count > 0 && root.visibleOperationCount === 0
        text: qsTr("No matching operations")
        color: Theme.textSecondary
    }

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
    }
    onCurrentItemChanged: {
        if (currentItem && currentIndex >= 0 && currentIndex < count)
            positionViewAtIndex(currentIndex, ListView.Contain);
    }

    Keys.onUpPressed: event => {
        filterModel.previousOperation(event.modifiers & Qt.ShiftModifier);
    }

    Keys.onDownPressed: event => {
        filterModel.nextOperation(event.modifiers & Qt.ShiftModifier);
    }

    Keys.onPressed: event => {
        if (event.key !== Qt.Key_PageUp && event.key !== Qt.Key_PageDown)
            return;

        const pageSize = Math.max(1, Math.floor(root.height / 50));
        const offset = event.key === Qt.Key_PageUp ? -pageSize : pageSize;
        filterModel.moveOperation(offset, event.modifiers & Qt.ShiftModifier);
        event.accepted = true;
    }

    delegate: OperationDelegate {
        id: operationDelegate
        required property int index
        width: root.width - scrollBar.width
        focused: root.currentIndex === index
        alternate: index % 2 === 0

        MouseArea {
            anchors.fill: parent
            onClicked: mouse => {
                if (mouse.modifiers & Qt.ControlModifier) {
                    // Cmd/Ctrl+click: toggle selection
                    filterModel.toggleSelectionAt(operationDelegate.index);
                } else {
                    filterModel.selectAt(operationDelegate.index, mouse.modifiers & Qt.ShiftModifier);
                }
            }
        }
    }
}
