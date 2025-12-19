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

                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    property var _category: modelData.category
                    property double _budgetLimit: _category?.budgetLimit || 0
                    property bool _isIncome: _budgetLimit > 0
                    property double _percentUsed: (modelData.amount / _budgetLimit) * 100.0

                    width: ListView.view.width
                    implicitHeight: contentColumn.implicitHeight + 24
                    color: delegateMouseArea.containsMouse ? Theme.surface : Theme.surfaceElevated
                    border.color: categoryListView.currentIndex === index ? Theme.accent : Theme.borderLight
                    border.width: categoryListView.currentIndex === index ? 2 : Theme.cardBorderWidth
                    radius: Theme.cardRadius

                    MouseArea {
                        id: delegateMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            categoryListView.currentIndex = index;
                            categoryDetailView.open();
                        }
                    }

                    ColumnLayout {
                        id: contentColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: Theme.spacingSmall

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: _category?.name || ""
                                font.pixelSize: Theme.fontSizeNormal
                                font.bold: true
                                color: Theme.textPrimary
                            }

                            Label {
                                text: _isIncome ? qsTr("(income)") : ""
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                            }

                            // Edit button
                            ToolButton {
                                text: "✏️"
                                font.pixelSize: Theme.fontSizeNormal
                                focusPolicy: Qt.NoFocus
                                opacity: hovered ? 1.0 : 0.5
                                onClicked: {
                                    categoryListView.currentIndex = index;
                                    categoryEditDialog.originalName = _category.name;
                                    categoryEditDialog.originalBudgetLimit = _budgetLimit;
                                    categoryEditDialog.open();
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Label {
                                text: {
                                    if (_isIncome && _percentUsed < 100)
                                        return qsTr("PENDING");
                                    if (!_isIncome && _percentUsed > 100)
                                        return qsTr("EXCEEDED");
                                    return "";
                                }
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: _isIncome ? Theme.warning : Theme.negative
                            }

                            Label {
                                text: {
                                    let base = Theme.formatAmount(Math.abs(modelData.amount)) + " / " + Theme.formatAmount(Math.abs(_budgetLimit));
                                    if (modelData.accumulated > 0) {
                                        return base + " (+" + Theme.formatAmount(modelData.accumulated) + ")";
                                    }
                                    return base;
                                }
                                font.pixelSize: Theme.fontSizeNormal
                                color: Theme.textSecondary
                            }
                        }

                        // Custom progress bar using rectangles
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 16
                            color: Theme.progressBackground
                            radius: 4

                            Rectangle {
                                width: Math.min(_percentUsed / 100, 1.0) * parent.width
                                height: parent.height
                                radius: 4
                                color: {
                                    if (_isIncome) {
                                        // Income: green when complete, warning when pending
                                        return _percentUsed >= 100 ? Theme.positive : Theme.warning;
                                    } else {
                                        // Expense: red when exceeded, warning when close
                                        if (_percentUsed > 100)
                                            return Theme.negative;
                                        if (_percentUsed > 80)
                                            return Theme.warning;
                                        return Theme.positive;
                                    }
                                }
                            }
                        }

                        Label {
                            property double _remaining: _isIncome ? (_budgetLimit - modelData.amount) : (modelData.amount - _budgetLimit)
                            text: {
                                let label, value;
                                if (_isIncome) {
                                    if (_remaining > 0) {
                                        label = qsTr("Expected: %1");
                                        value = _remaining;
                                    } else {
                                        label = qsTr("Received: %1 extra");
                                        value = -_remaining;
                                    }
                                } else {
                                    if (_remaining >= 0) {
                                        label = qsTr("Remaining: %1");
                                        value = _remaining;
                                    } else {
                                        label = qsTr("Exceeded: %1");
                                        value = -_remaining;
                                    }
                                }
                                return label.arg(Theme.formatAmount(value));
                            }
                            font.pixelSize: Theme.fontSizeSmall
                            color: {
                                if (_isIncome) {
                                    return _remaining > 0 ? Theme.warning : Theme.positive;
                                } else {
                                    return _remaining >= 0 ? Theme.textSecondary : Theme.negative;
                                }
                            }
                        }
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
