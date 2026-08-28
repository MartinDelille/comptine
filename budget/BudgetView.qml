pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui
import Comptine

FocusScope {
    id: root

    property bool dialogOpen: categoryEditDialog.visible

    function editCurrentCategory() {
        let category = CategoryController.current;
        if (category) {
            categoryEditDialog.edit(category);
        }
    }

    function addCategory() {
        categoryEditDialog.edit();
    }

    CategoryEditDialog {
        id: categoryEditDialog
        date: BudgetData.budgetDate
        onCategoryEdited: function (category, newName, newBudgetLimit) {
            CategoryEditor.edit(newName, newBudgetLimit, category, date);
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

        // Summary
        ColumnLayout {
            RowLayout {
                Label {
                    text: qsTr("Total Budget:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                Label {
                    property real _balance: CategoryController.totalIncome - CategoryController.totalExpense
                    text: `${Theme.formatAmountWithoutCurrency(CategoryController.totalIncome)} - ${Theme.formatAmountWithoutCurrency(CategoryController.totalExpense)} = ${Theme.formatAmount(_balance)}`
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: _balance == 0 ? Theme.textMuted : Theme.negative
                }
            }

            RowLayout {
                spacing: Theme.spacingSmall

                Label {
                    text: qsTr("To Save:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                AmountLabel {
                    amount: CategoryController.totalToSave
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            RowLayout {
                spacing: Theme.spacingSmall

                Label {
                    text: qsTr("To Leftover:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                AmountLabel {
                    amount: CategoryController.totalToReport
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            RowLayout {
                spacing: Theme.spacingSmall
                visible: CategoryController.totalFromReport > 0

                Label {
                    text: qsTr("From Leftover:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                AmountLabel {
                    amount: CategoryController.totalFromReport
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.warning
                }
            }

            RowLayout {
                spacing: Theme.spacingSmall

                Label {
                    text: qsTr("Net:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                AmountLabel {
                    amount: CategoryController.netReport
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            RowLayout {
                spacing: Theme.spacingSmall

                Label {
                    text: qsTr("Balanced:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                Label {
                    text: `${CategoryController.balancedCount} / ${CategoryController.count}`
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
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
            onCurrentIndexChanged: CategoryController.currentIndex = currentIndex

            Keys.onReturnPressed: categoryDetailView.open()
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
                    categoryListView.currentIndex = index;
                    categoryDetailView.open();
                }

                onEditClicked: {
                    categoryListView.currentIndex = index;
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
