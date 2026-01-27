import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

FocusScope {
    id: root

    property bool dialogOpen: categoryEditDialog.visible

    function editCurrentCategory() {
        let category = AppState.categories.current;
        if (category) {
            categoryEditDialog.originalName = category.name;
            categoryEditDialog.originalBudgetLimit = category.budgetLimit;
            categoryEditDialog.open();
        }
    }

    function addCategory() {
        categoryEditDialog.originalName = "";
        categoryEditDialog.originalBudgetLimit = 0;
        categoryEditDialog.open();
    }

    CategoryEditDialog {
        id: categoryEditDialog
    }

    CategoryDetailView {
        id: categoryDetailView
    }

    LeftoverDialog {
        id: leftoverDialog
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXLarge
            spacing: Theme.spacingXLarge

            // Month navigation
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                RowLayout {
                    id: monthSelector

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Theme.spacingXLarge

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
                        Layout.preferredWidth: 200
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

                Button {
                    text: qsTr("Leftover...")
                    onClicked: leftoverDialog.open()
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
                        categoryEditDialog.originalName = category.name;
                        categoryEditDialog.originalBudgetLimit = budgetLimit;
                        categoryEditDialog.open();
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
