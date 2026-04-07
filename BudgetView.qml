pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Comptine

FocusScope {
    id: root

    property bool dialogOpen: categoryEditDialog.visible

    function editCurrentCategory() {
        let category = AppState.categories.current;
        if (category) {
            categoryEditDialog.edit(category);
        }
    }

    function addCategory() {
        categoryEditDialog.edit();
    }

    CategoryEditDialog {
        id: categoryEditDialog
    }

    CategoryDetailView {
        id: categoryDetailView
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXLarge
            spacing: Theme.spacingXLarge

            // Month navigation and summary
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                // Month selector
                RowLayout {
                    spacing: Theme.spacingNormal

                    Button {
                        text: "<"
                        focusPolicy: Qt.NoFocus
                        onClicked: AppState.navigation.previousMonth()
                        implicitWidth: 40
                    }

                    DateLabel {
                        date: AppState.navigation.budgetDate
                        color: Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.preferredWidth: 150
                    }

                    Button {
                        text: ">"
                        focusPolicy: Qt.NoFocus
                        onClicked: AppState.navigation.nextMonth()
                        implicitWidth: 40
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                // Leftover summary (inline)
                RowLayout {
                    spacing: Theme.spacingLarge

                    RowLayout {
                        spacing: Theme.spacingSmall

                        Label {
                            text: qsTr("Total Budget:")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                        Label {
                            property real _balance: AppState.categories.totalIncome - AppState.categories.totalExpense
                            text: `${Theme.formatAmountWithoutCurrency(AppState.categories.totalIncome)} - ${Theme.formatAmountWithoutCurrency(AppState.categories.totalExpense)} = ${Theme.formatAmount(_balance)}`
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
                        Label {
                            text: Theme.formatAmount(AppState.categories.totalToSave)
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.positive
                        }
                    }

                    RowLayout {
                        spacing: Theme.spacingSmall

                        Label {
                            text: qsTr("To Leftover:")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                        Label {
                            text: Theme.formatAmount(AppState.categories.totalToReport)
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.accent
                        }
                    }

                    RowLayout {
                        spacing: Theme.spacingSmall
                        visible: AppState.categories.totalFromReport > 0

                        Label {
                            text: qsTr("From Leftover:")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                        Label {
                            text: Theme.formatAmount(AppState.categories.totalFromReport)
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
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
                        Label {
                            text: Theme.formatAmount(AppState.categories.netReport)
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.amountColor(AppState.categories.netReport)
                        }
                    }
                }
            }

            ListView {
                id: categoryListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: AppState.categories
                spacing: Theme.spacingNormal
                clip: true
                focus: true
                currentIndex: AppState.navigation.currentCategoryIndex
                onCurrentIndexChanged: AppState.navigation.currentCategoryIndex = currentIndex

                Keys.onReturnPressed: categoryDetailView.open()

                delegate: MonthCategoryItem {
                    width: ListView.view.width
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
                visible: AppState.categories.count === 0
                text: qsTr("No categories defined")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.textMuted
            }
        }
    }
}
