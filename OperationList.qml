import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    id: root

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    focus: true

    Keys.onUpPressed: {
        if (currentIndex > 0) {
            currentIndex--;
            positionViewAtIndex(currentIndex, ListView.Contain);
        }
    }

    Keys.onDownPressed: {
        if (currentIndex < count - 1) {
            currentIndex++;
            positionViewAtIndex(currentIndex, ListView.Contain);
        }
    }

    delegate: OperationDelegate {
        required property int index
        width: root.width - scrollBar.width
        operation: budgetData.getOperation(index)
        balance: budgetData.balanceAtIndex(index)
        selected: root.currentIndex === index
        alternate: index % 2 === 0

        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.currentIndex = parent.index;
                root.forceActiveFocus();
            }
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
    }
}
