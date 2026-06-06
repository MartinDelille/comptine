import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

Rectangle {
    id: root

    required property var categories
    required property var navigation
    required property var modelData
    required property int index
    required property bool isCurrentItem

    property var category: modelData.category
    property double amount: modelData.amount || 0
    property double budgetLimit: modelData.budgetLimit || 0
    property bool isIncome: budgetLimit > 0
    property double percentUsed: (modelData.amount / budgetLimit) * 100.0

    // Leftover data from model
    property double leftover: modelData.leftover || 0
    property double saveAmount: modelData.saveAmount || 0
    property double reportAmount: modelData.reportAmount || 0
    property double accumulated: modelData.accumulated || 0

    // Computed: remaining leftover after allocations
    readonly property double remainingLeftover: leftover - saveAmount - reportAmount
    readonly property bool isBalanced: Math.abs(remainingLeftover) < 0.01

    signal clicked
    signal editClicked

    implicitHeight: contentColumn.implicitHeight + 24
    color: delegateMouseArea.containsMouse ? Theme.surface : Theme.surfaceElevated
    border.color: isBalanced ? Theme.positive : (isCurrentItem ? Theme.accent : Theme.borderLight)
    border.width: isCurrentItem ? 2 : Theme.cardBorderWidth
    radius: Theme.cardRadius

    MouseArea {
        id: delegateMouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 12
        spacing: Theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: root.category?.name || ""
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
                color: Theme.textPrimary
            }

            Label {
                text: root.isIncome ? qsTr("(income)") : ""
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textMuted
            }

            // Edit button
            ToolButton {
                text: "✏️"
                font.pixelSize: Theme.fontSizeNormal
                focusPolicy: Qt.NoFocus
                opacity: hovered ? 1.0 : 0.5
                onClicked: root.editClicked()
            }

            Item {
                Layout.fillWidth: true
            }

            // Leftover display and allocation fields (inline)
            Label {
                text: Theme.formatAmount(root.remainingLeftover)
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: root.remainingLeftover >= 0 ? Theme.positive : Theme.negative
                visible: root.leftover !== 0
            }

            // Save amount field
            RowLayout {
                spacing: 2

                Label {
                    text: qsTr("Save:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                AmountField {
                    id: saveField
                    Layout.preferredWidth: 70
                    value: root.saveAmount

                    function applyValue(newValue) {
                        let maxSave = root.saveAmount + root.remainingLeftover;
                        let clampedValue = Math.max(0, Math.min(newValue, maxSave));
                        root.categories.setSaveAmount(root.category, root.navigation.budgetDate, newValue);
                    }

                    onEdited: newValue => applyValue(newValue)
                }

                ToolButton {
                    property bool canDiscard: root.saveAmount != 0
                    text: canDiscard ? "✖" : "⬆"
                    font.pixelSize: Theme.fontSizeSmall
                    enabled: !(root.isBalanced && root.saveAmount === 0)
                    opacity: enabled ? 1.0 : 0.3
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

                    onClicked: {
                        if (canDiscard) {
                            root.categories.setSaveAmount(root.category, root.navigation.budgetDate, 0);
                        } else {
                            let newSave = root.saveAmount + root.remainingLeftover;
                            root.categories.setSaveAmount(root.category, root.navigation.budgetDate, newSave);
                        }
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: canDiscard ? qsTr("Discard") : qsTr("Allocate remaining to Save")
                    ToolTip.delay: 500
                }
            }

            // Report amount field
            RowLayout {
                spacing: 2

                Label {
                    text: qsTr("Report:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                AmountField {
                    id: reportField
                    Layout.preferredWidth: 70
                    value: root.reportAmount

                    function applyValue(newValue) {
                        let clampedValue;
                        if (root.leftover >= 0) {
                            let maxReport = root.reportAmount + root.remainingLeftover;
                            clampedValue = Math.max(0, Math.min(newValue, maxReport));
                        } else {
                            clampedValue = Math.max(root.leftover, Math.min(newValue, 0));
                        }
                        root.categories.setReportAmount(root.category, root.navigation.budgetDate, newValue);
                    }

                    onEdited: newValue => applyValue(newValue)
                }

                ToolButton {
                    property bool canDiscard: root.reportAmount != 0
                    text: canDiscard ? "✖" : "⬆"
                    font.pixelSize: Theme.fontSizeSmall
                    enabled: !(root.isBalanced && root.reportAmount === 0)
                    opacity: enabled ? 1.0 : 0.3
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20

                    onClicked: {
                        if (canDiscard) {
                            root.categories.setReportAmount(root.category, root.navigation.budgetDate, 0);
                        } else {
                            if (root.leftover >= 0) {
                                let newReport = root.reportAmount + root.remainingLeftover;
                                root.categories.setReportAmount(root.category, root.navigation.budgetDate, newReport);
                            } else {
                                root.categories.setReportAmount(root.category, root.navigation.budgetDate, root.leftover);
                            }
                        }
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: canDiscard ? qsTr("Clear report") : (root.leftover >= 0 ? qsTr("Allocate remaining to Report") : qsTr("Carry forward deficit"))
                    ToolTip.delay: 500
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 16
            color: Theme.progressBackground
            radius: 4

            BudgetProgress {
                value: root.amount + (root.reportAmount + root.saveAmount) * (root.isIncome ? 1 : -1)
                total: root.budgetLimit
                color: "#ff8080"
            }

            BudgetProgress {
                value: root.amount + root.reportAmount * (root.isIncome ? 1 : -1)
                total: root.budgetLimit
                color: Theme.accent
            }

            BudgetProgress {
                value: root.amount
                total: root.budgetLimit
                color: {
                    if (root.isIncome) {
                        // Income: green when complete, warning when pending
                        return root.amount > root.budgetLimit ? Theme.positive : Theme.warning;
                    } else {
                        // Expense: red when exceeded, warning when close
                        if (root.amount < root.budgetLimit)
                            return Theme.negative;
                        if (root.amount < 0.8 * root.budgetLimit)
                            return Theme.warning;
                        return Theme.positive;
                    }
                }
            }
        }

        RowLayout {
            Label {
                property double remaining: root.isIncome ? (root.budgetLimit - root.amount) : (root.amount - root.budgetLimit)
                text: {
                    let label, value;
                    if (root.isIncome) {
                        if (remaining > 0) {
                            label = qsTr("Expected: %1");
                            value = remaining;
                        } else {
                            label = qsTr("Received: %1 extra");
                            value = -remaining;
                        }
                    } else {
                        if (remaining >= 0) {
                            label = qsTr("Remaining: %1");
                            value = remaining;
                        } else {
                            label = qsTr("Exceeded: %1");
                            value = -remaining;
                        }
                    }
                    return label.arg(Theme.formatAmount(value));
                }
                font.pixelSize: Theme.fontSizeSmall
                color: {
                    if (root.isIncome) {
                        return remaining > 0 ? Theme.warning : Theme.positive;
                    } else {
                        return remaining >= 0 ? Theme.textSecondary : Theme.negative;
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: {
                    if (root.isIncome) {
                        if (root.amount > root.budgetLimit) {
                            return qsTr("PENDING");
                        }
                    } else if (root.amount < root.budgetLimit) {
                        return qsTr("EXCEEDED");
                    }
                    return "";
                }
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: root.isIncome ? Theme.warning : Theme.negative
            }

            Label {
                text: Theme.formatAmount(Math.abs(root.amount)) + " / " + Theme.formatAmount(Math.abs(root.budgetLimit)) + " (" + (root.accumulated > 0 ? "+" : "") + Theme.formatAmount(root.accumulated) + ")"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
        }
    }
}
