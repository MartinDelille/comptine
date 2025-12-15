import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    id: root

    title: qsTr("Monthly Leftover Summary")
    standardButtons: Dialog.Close

    width: Math.min(700, parent.width * 0.9)
    height: Math.min(550, parent.height * 0.85)

    // Alias to the C++ model for convenience
    property var leftoverModel: AppState.categories.leftoverModel

    function open() {
        leftoverModel.year = AppState.navigation.budgetYear;
        leftoverModel.month = AppState.navigation.budgetMonth;
        leftoverModel.refresh();
        visible = true;
    }

    function monthName(m) {
        let monthNames = [qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"), qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"), qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")];
        return monthNames[m - 1] || "";
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingLarge

        // Header with month info
        Label {
            text: qsTr("%1 %2").arg(monthName(leftoverModel.month)).arg(leftoverModel.year)
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.textPrimary
        }

        // Column headers
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("Category")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Leftover")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 70
                horizontalAlignment: Text.AlignRight
            }
            Label {
                text: qsTr("Save")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 105
                horizontalAlignment: Text.AlignCenter
            }
            Label {
                text: qsTr("Report")
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
                Layout.preferredWidth: 105
                horizontalAlignment: Text.AlignCenter
            }
            // Space for balanced indicator
            Item {
                Layout.preferredWidth: 24
            }
        }

        // Category list with leftover info
        ListView {
            id: leftoverList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: leftoverModel
            spacing: Theme.spacingSmall

            delegate: Rectangle {
                id: delegateRoot
                required property int index
                required property string name
                required property double budgetLimit
                required property double spent
                required property double accumulated
                required property double leftover
                required property double saveAmount
                required property double reportAmount
                required property bool isBalanced

                // Remaining leftover = original leftover - allocated amounts
                readonly property double remainingLeftover: leftover - saveAmount - reportAmount

                width: ListView.view.width
                height: contentRow.implicitHeight + Theme.spacingSmall * 2
                radius: Theme.cardRadius
                border.width: Theme.cardBorderWidth
                border.color: delegateRoot.isBalanced ? Theme.positive : Theme.borderLight
                color: delegateRoot.isBalanced ? Qt.rgba(Theme.positive.r, Theme.positive.g, Theme.positive.b, 0.05) : Theme.surfaceElevated

                RowLayout {
                    id: contentRow
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSmall
                    spacing: Theme.spacingNormal

                    // Category info
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        Label {
                            text: delegateRoot.name
                            font.pixelSize: Theme.fontSizeNormal
                            font.bold: true
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            spacing: Theme.spacingSmall

                            Label {
                                text: qsTr("Budget: %1").arg(Theme.formatAmount(delegateRoot.budgetLimit))
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                            }

                            Label {
                                text: "|"
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                            }

                            Label {
                                text: qsTr("Spent: %1").arg(Theme.formatAmount(delegateRoot.spent))
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                            }

                            Label {
                                visible: delegateRoot.accumulated > 0
                                text: "|"
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textMuted
                            }

                            Label {
                                visible: delegateRoot.accumulated > 0
                                text: qsTr("Carried: +%1").arg(Theme.formatAmount(delegateRoot.accumulated))
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.positive
                            }
                        }
                    }

                    // Remaining leftover amount (decreases as Save/Report are allocated)
                    Label {
                        text: Theme.formatAmount(delegateRoot.remainingLeftover)
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        color: delegateRoot.remainingLeftover >= 0 ? Theme.positive : Theme.negative
                        Layout.preferredWidth: 70
                        horizontalAlignment: Text.AlignRight
                    }

                    // Save amount field with top-up button
                    RowLayout {
                        Layout.preferredWidth: 105
                        spacing: 2

                        AmountField {
                            id: saveField
                            Layout.fillWidth: true
                            value: delegateRoot.saveAmount
                            enabled: delegateRoot.remainingLeftover > 0 || delegateRoot.saveAmount > 0

                            function applyValue(newValue) {
                                // Clamp: can allocate up to current save + remaining leftover
                                let maxSave = delegateRoot.saveAmount + delegateRoot.remainingLeftover;
                                let clampedValue = Math.max(0, Math.min(newValue, maxSave));
                                AppState.categories.setLeftoverAmounts(delegateRoot.name, leftoverModel.year, leftoverModel.month, clampedValue, delegateRoot.reportAmount);
                            }

                            onEdited: newValue => applyValue(newValue)
                            onLiveEdited: newValue => applyValue(newValue)
                            onActiveFocusChanged: if (activeFocus)
                                leftoverList.positionViewAtIndex(delegateRoot.index, ListView.Contain)
                        }

                        ToolButton {
                            id: saveTopButton
                            text: "⬆"
                            font.pixelSize: Theme.fontSizeSmall
                            enabled: delegateRoot.remainingLeftover > 0
                            opacity: enabled ? 1.0 : 0.3
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            padding: 0

                            onClicked: {
                                let newSave = delegateRoot.saveAmount + delegateRoot.remainingLeftover;
                                AppState.categories.setLeftoverAmounts(delegateRoot.name, leftoverModel.year, leftoverModel.month, newSave, delegateRoot.reportAmount);
                            }

                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Allocate remaining to Save")
                            ToolTip.delay: 500
                        }
                    }

                    // Report amount field with top-up button
                    RowLayout {
                        Layout.preferredWidth: 105
                        spacing: 2

                        AmountField {
                            id: reportField
                            Layout.fillWidth: true
                            value: delegateRoot.reportAmount
                            enabled: delegateRoot.remainingLeftover !== 0 || delegateRoot.reportAmount !== 0

                            function applyValue(newValue) {
                                let clampedValue;
                                if (delegateRoot.leftover >= 0) {
                                    // Positive leftover: can allocate up to current report + remaining
                                    let maxReport = delegateRoot.reportAmount + delegateRoot.remainingLeftover;
                                    clampedValue = Math.max(0, Math.min(newValue, maxReport));
                                } else {
                                    // Negative leftover (overspending): report in range [leftover, 0]
                                    // User can choose to report none (0) or carry debt forward (negative)
                                    clampedValue = Math.max(delegateRoot.leftover, Math.min(newValue, 0));
                                }
                                AppState.categories.setLeftoverAmounts(delegateRoot.name, leftoverModel.year, leftoverModel.month, delegateRoot.saveAmount, clampedValue);
                            }

                            onEdited: newValue => applyValue(newValue)
                            onLiveEdited: newValue => applyValue(newValue)
                            onActiveFocusChanged: if (activeFocus)
                                leftoverList.positionViewAtIndex(delegateRoot.index, ListView.Contain)
                        }

                        ToolButton {
                            id: reportTopButton
                            text: "⬆"
                            font.pixelSize: Theme.fontSizeSmall
                            // Enable for positive remaining OR negative leftover that hasn't been fully reported
                            enabled: delegateRoot.remainingLeftover > 0 || (delegateRoot.leftover < 0 && delegateRoot.reportAmount > delegateRoot.leftover)
                            opacity: enabled ? 1.0 : 0.3
                            Layout.preferredWidth: 24
                            Layout.preferredHeight: 24
                            padding: 0

                            onClicked: {
                                if (delegateRoot.leftover >= 0) {
                                    // Positive leftover: allocate remaining to report
                                    let newReport = delegateRoot.reportAmount + delegateRoot.remainingLeftover;
                                    AppState.categories.setLeftoverAmounts(delegateRoot.name, leftoverModel.year, leftoverModel.month, delegateRoot.saveAmount, newReport);
                                } else {
                                    // Negative leftover: set report to full negative amount (carry forward debt)
                                    AppState.categories.setLeftoverAmounts(delegateRoot.name, leftoverModel.year, leftoverModel.month, delegateRoot.saveAmount, delegateRoot.leftover);
                                }
                            }

                            ToolTip.visible: hovered
                            ToolTip.text: delegateRoot.leftover >= 0 ? qsTr("Allocate remaining to Report") : qsTr("Carry forward deficit")
                            ToolTip.delay: 500
                        }
                    }

                    // Balanced indicator
                    Label {
                        text: delegateRoot.isBalanced ? "✓" : ""
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.positive
                        Layout.preferredWidth: 24
                        horizontalAlignment: Text.AlignCenter
                    }
                }
            }
        }

        // Summary section
        Rectangle {
            Layout.fillWidth: true
            height: summaryColumn.implicitHeight + Theme.spacingLarge * 2
            radius: Theme.cardRadius
            border.width: Theme.cardBorderWidth
            border.color: Theme.accent
            color: Theme.surface

            ColumnLayout {
                id: summaryColumn
                anchors.fill: parent
                anchors.margins: Theme.spacingLarge
                spacing: Theme.spacingNormal

                Label {
                    text: qsTr("Monthly Summary")
                    font.pixelSize: Theme.fontSizeNormal
                    font.bold: true
                    color: Theme.textPrimary
                }

                GridLayout {
                    columns: 2
                    columnSpacing: Theme.spacingLarge
                    rowSpacing: Theme.spacingSmall

                    Label {
                        text: qsTr("To Savings:")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }
                    Label {
                        text: Theme.formatAmount(leftoverModel.totalToSave)
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        color: Theme.positive
                    }

                    Label {
                        text: qsTr("To Leftover Account:")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }
                    Label {
                        text: Theme.formatAmount(leftoverModel.totalToReport)
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        color: Theme.accent
                    }

                    Label {
                        text: qsTr("From Leftover Account:")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }
                    Label {
                        text: Theme.formatAmount(leftoverModel.totalFromReport)
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        color: leftoverModel.totalFromReport > 0 ? Theme.warning : Theme.textSecondary
                    }

                    Rectangle {
                        Layout.columnSpan: 2
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.border
                    }

                    Label {
                        text: qsTr("Net Leftover Transfer:")
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Label {
                        text: Theme.formatAmount(leftoverModel.netReport)
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        color: Theme.amountColor(leftoverModel.netReport)
                    }
                }
            }
        }
    }
}
