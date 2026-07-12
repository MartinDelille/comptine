pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

FocusScope {
    id: root

    required property var categories
    required property var navigation
    property bool dialogOpen: categoryEditDialog.visible

    function editCurrentCategory() {
        let category = categories.current;
        if (category) {
            categoryEditDialog.edit(category);
        }
    }

    function addCategory() {
        categoryEditDialog.edit();
    }

    CategoryEditDialog {
        id: categoryEditDialog
        date: root.navigation.budgetDate
        onCategoryEdited: function (category, newName, newBudgetLimit) {
            root.categories.editCategory(newName, newBudgetLimit, category, date);
        }
    }

    CategoryDetailView {
        id: categoryDetailView
        category: root.categories.current
        date: root.navigation.budgetDate

        onOpened: {
            operations = root.categories.operationsForCategory(category, date);
        }

        onNavigateToOperation: function (operation) {
            root.navigation.navigateToOperation(operation);
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
                onClicked: root.navigation.previousMonth()
            }

            DateLabel {
                date: root.navigation.budgetDate
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                Layout.preferredWidth: 150
            }

            Button {
                text: ">"
                focusPolicy: Qt.NoFocus
                onClicked: root.navigation.nextMonth()
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
                    property real _balance: root.categories.totalIncome - root.categories.totalExpense
                    text: `${Theme.formatAmountWithoutCurrency(root.categories.totalIncome)} - ${Theme.formatAmountWithoutCurrency(root.categories.totalExpense)} = ${Theme.formatAmount(_balance)}`
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
                    amount: root.categories.totalToSave
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
                    amount: root.categories.totalToReport
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            RowLayout {
                spacing: Theme.spacingSmall
                visible: root.categories.totalFromReport > 0

                Label {
                    text: qsTr("From Leftover:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }
                AmountLabel {
                    amount: root.categories.totalFromReport
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
                    amount: root.categories.netReport
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
                    text: `${root.categories.balancedCount} / ${root.categories.count}`
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                }
            }
        }

        ListView {
            id: categoryListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.categories
            spacing: Theme.spacingNormal
            clip: true
            focus: true
            currentIndex: root.navigation.currentCategoryIndex
            onCurrentIndexChanged: root.navigation.currentCategoryIndex = currentIndex

            Keys.onReturnPressed: categoryDetailView.open()
            ScrollBar.vertical: ScrollBar {
                id: scrollBar
            }

            delegate: MonthCategoryItem {
                categories: root.categories
                navigation: root.navigation
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
            visible: root.categories.count === 0
            text: qsTr("No categories defined")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: Theme.fontSizeLarge
            color: Theme.textMuted
        }
    }
}
