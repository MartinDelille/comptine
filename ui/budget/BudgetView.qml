pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common
import services
import editor

FocusScope {
    id: root

    property bool dialogOpen: categoryEditDialog.visible
    readonly property var currentCategoryItem: categoryListView.currentItem
    readonly property real netBudget: CategoryController.totalIncome - CategoryController.totalExpense
    readonly property real netReported: CategoryController.netReport

    function editCurrentCategory() {
        let category = CategoryController.current;
        if (category) {
            categoryEditDialog.edit(category);
        }
    }

    function addCategory() {
        categoryEditDialog.edit();
    }

    function movePage(direction) {
        const itemHeight = categoryListView.currentItem?.implicitHeight || 100;
        const pageSize = Math.max(1, Math.floor(categoryListView.height / itemHeight));
        CategoryController.currentIndex = Math.max(0, Math.min(CategoryController.count - 1, CategoryController.currentIndex + direction * pageSize));
    }

    function moveToUnbalanced(direction) {
        let index = CategoryController.currentIndex;
        if (index < 0)
            index = direction > 0 ? -1 : CategoryController.count;

        index += direction;
        while (index >= 0 && index < CategoryController.count) {
            if (!CategoryController.isBalanced(index)) {
                CategoryController.currentIndex = index;
                return;
            }
            index += direction;
        }
    }

    function saveAvailable() {
        let categoryItem = root.currentCategoryItem;
        if (!categoryItem || !categoryItem.category)
            return;

        let amount = categoryItem.saveAmount !== 0 ? 0 : categoryItem.saveAmount + categoryItem.remainingLeftover;
        CategoryEditor.setSaveAmount(categoryItem.category, BudgetData.budgetDate, amount);
    }

    function reportAvailable() {
        let categoryItem = root.currentCategoryItem;
        if (!categoryItem || !categoryItem.category)
            return;

        let amount;
        if (categoryItem.reportAmount !== 0) {
            amount = 0;
        } else if (categoryItem.leftover >= 0) {
            amount = categoryItem.reportAmount + categoryItem.remainingLeftover;
        } else {
            amount = categoryItem.leftover;
        }
        CategoryEditor.setReportAmount(categoryItem.category, BudgetData.budgetDate, amount);
    }

    CategoryEditDialog {
        id: categoryEditDialog
        date: BudgetData.budgetDate
        onCategoryEdited: function (category, newName, newBudgetLimit, inheritPrevious) {
            CategoryEditor.edit(newName, newBudgetLimit, category, date, inheritPrevious);
        }
    }

    CategoryDetailView {
        id: categoryDetailView
        category: CategoryController.current
        date: BudgetData.budgetDate

        onOpened: {
            operations = CategoryController.operationsForCategory(category, date);
        }

        onNavigateToOperation: function (operation) {
            BudgetData.navigateToOperation(operation);
        }
    }

    ColumnLayout {
        anchors.fill: parent

        // Month selector
        RowLayout {
            spacing: Theme.spacingNormal
            Layout.alignment: Qt.AlignHCenter

            Button {
                text: "<"
                focusPolicy: Qt.NoFocus
                onClicked: BudgetData.previousMonth()
            }

            DateLabel {
                date: BudgetData.budgetDate
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                Layout.preferredWidth: 150
            }

            Button {
                text: ">"
                focusPolicy: Qt.NoFocus
                onClicked: BudgetData.nextMonth()
            }
        }

        // Monthly overview
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: overviewColumn.implicitHeight + 24
            color: Theme.surface
            border.color: Theme.border
            border.width: Theme.cardBorderWidth
            radius: Theme.cardRadius

            ColumnLayout {
                id: overviewColumn
                anchors.fill: parent
                anchors.margins: 12
                spacing: Theme.spacingSmall

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text: qsTr("Monthly overview")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLarge
                        font.weight: Font.Medium
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("%1 of %2 categories balanced").arg(CategoryController.balancedCount).arg(CategoryController.count)
                        color: CategoryController.balancedCount === CategoryController.count ? Theme.positive : Theme.warning
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Label {
                        text: qsTr("Income")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    AmountLabel {
                        amount: CategoryController.totalIncome
                        font.pixelSize: Theme.fontSizeNormal
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Expenses")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    AmountLabel {
                        amount: CategoryController.totalExpense
                        font.pixelSize: Theme.fontSizeNormal
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Net")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    AmountLabel {
                        amount: root.netBudget
                        color: root.netBudget >= 0 ? Theme.positive : Theme.negative
                        font.pixelSize: Theme.fontSizeNormal
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSmall

                    Label {
                        text: qsTr("To save")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    AmountLabel {
                        amount: CategoryController.totalToSave
                        font.pixelSize: Theme.fontSizeSmall
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("To report")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    AmountLabel {
                        amount: CategoryController.totalToReport
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeSmall
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Reported balance")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    AmountLabel {
                        amount: root.netReported
                        color: root.netReported >= 0 ? Theme.accent : Theme.warning
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }
        }

        ListView {
            id: categoryListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: CategoryController
            spacing: Theme.spacingNormal
            clip: true
            focus: true
            currentIndex: CategoryController.currentIndex

            function ensureCurrentCategoryVisible() {
                if (count > 0 && currentIndex >= 0)
                    positionViewAtIndex(currentIndex, ListView.Contain);
            }

            onCountChanged: ensureCurrentCategoryVisible()
            onCurrentIndexChanged: ensureCurrentCategoryVisible()

            Keys.onReturnPressed: categoryDetailView.open()
            Keys.onSpacePressed: function (event) {
                event.accepted = true;
                categoryDetailView.open();
            }
            ScrollBar.vertical: ScrollBar {
                id: scrollBar
            }

            delegate: MonthCategoryItem {
                categories: CategoryController
                budgetData: BudgetData
                categoryEditor: CategoryEditor
                width: ListView.view.width - scrollBar.width
                isCurrentItem: categoryListView.currentIndex === index

                onClicked: {
                    CategoryController.currentIndex = index;
                    categoryDetailView.open();
                }

                onEditClicked: {
                    CategoryController.currentIndex = index;
                    categoryEditDialog.edit(category);
                }
            }
        }

        // Empty state
        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: CategoryController.count === 0
            text: qsTr("No categories defined")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.textMuted
        }
    }
}
