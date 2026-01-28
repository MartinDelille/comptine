import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var modelData
    required property int index
    required property bool isCurrentItem

    property var category: modelData.category
    property double budgetLimit: category?.budgetLimit || 0
    property bool isIncome: budgetLimit > 0
    property double percentUsed: (modelData.amount / budgetLimit) * 100.0

    // Leftover data from model
    property double leftover: modelData.leftover || 0
    property double saveAmount: modelData.saveAmount || 0
    property double reportAmount: modelData.reportAmount || 0

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
                visible: root.leftover > 0 || root.saveAmount > 0

                Label {
                    text: qsTr("Save:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                AmountField {
                    id: saveField
                    Layout.preferredWidth: 70
                    value: root.saveAmount
                    enabled: root.remainingLeftover > 0 || root.saveAmount > 0

                    function applyValue(newValue) {
                        let maxSave = root.saveAmount + root.remainingLeftover;
                        let clampedValue = Math.max(0, Math.min(newValue, maxSave));
                        AppState.categories.setLeftoverAmounts(root.category.name, AppState.navigation.budgetDate, clampedValue, root.reportAmount);
                    }

                    onEdited: newValue => applyValue(newValue)
                    onLiveEdited: newValue => applyValue(newValue)
                }

                ToolButton {
                    text: "⬆"
                    font.pixelSize: Theme.fontSizeSmall
                    enabled: root.remainingLeftover > 0
                    opacity: enabled ? 1.0 : 0.3
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    padding: 0
                    focusPolicy: Qt.NoFocus

                    onClicked: {
                        let newSave = root.saveAmount + root.remainingLeftover;
                        AppState.categories.setLeftoverAmounts(root.category.name, AppState.navigation.budgetDate, newSave, root.reportAmount);
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Allocate remaining to Save")
                    ToolTip.delay: 500
                }
            }

            // Report amount field
            RowLayout {
                spacing: 2
                visible: root.leftover !== 0 || root.reportAmount !== 0

                Label {
                    text: qsTr("Report:")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                AmountField {
                    id: reportField
                    Layout.preferredWidth: 70
                    value: root.reportAmount
                    enabled: root.remainingLeftover !== 0 || root.reportAmount !== 0

                    function applyValue(newValue) {
                        let clampedValue;
                        if (root.leftover >= 0) {
                            let maxReport = root.reportAmount + root.remainingLeftover;
                            clampedValue = Math.max(0, Math.min(newValue, maxReport));
                        } else {
                            clampedValue = Math.max(root.leftover, Math.min(newValue, 0));
                        }
                        AppState.categories.setLeftoverAmounts(root.category.name, AppState.navigation.budgetDate, root.saveAmount, clampedValue);
                    }

                    onEdited: newValue => applyValue(newValue)
                    onLiveEdited: newValue => applyValue(newValue)
                }

                ToolButton {
                    text: "⬆"
                    font.pixelSize: Theme.fontSizeSmall
                    enabled: root.remainingLeftover > 0 || (root.leftover < 0 && root.reportAmount > root.leftover)
                    opacity: enabled ? 1.0 : 0.3
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    padding: 0
                    focusPolicy: Qt.NoFocus

                    onClicked: {
                        if (root.leftover >= 0) {
                            let newReport = root.reportAmount + root.remainingLeftover;
                            AppState.categories.setLeftoverAmounts(root.category.name, AppState.navigation.budgetDate, root.saveAmount, newReport);
                        } else {
                            AppState.categories.setLeftoverAmounts(root.category.name, AppState.navigation.budgetDate, root.saveAmount, root.leftover);
                        }
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: root.leftover >= 0 ? qsTr("Allocate remaining to Report") : qsTr("Carry forward deficit")
                    ToolTip.delay: 500
                }
            }

            // Balanced indicator
            Label {
                text: root.isBalanced ? "✓" : ""
                font.pixelSize: Theme.fontSizeNormal
                color: Theme.positive
                visible: root.leftover !== 0
            }

            Label {
                text: {
                    if (root.isIncome && root.percentUsed < 100)
                        return qsTr("PENDING");
                    if (!root.isIncome && root.percentUsed > 100)
                        return qsTr("EXCEEDED");
                    return "";
                }
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: root.isIncome ? Theme.warning : Theme.negative
            }

            Label {
                text: {
                    let base = Theme.formatAmount(Math.abs(root.modelData.amount)) + " / " + Theme.formatAmount(Math.abs(root.budgetLimit));
                    if (root.modelData.accumulated > 0) {
                        return base + " (+" + Theme.formatAmount(root.modelData.accumulated) + ")";
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
                width: Math.min(root.percentUsed / 100, 1.0) * parent.width
                height: parent.height
                radius: 4
                color: {
                    if (root.isIncome) {
                        // Income: green when complete, warning when pending
                        return root.percentUsed >= 100 ? Theme.positive : Theme.warning;
                    } else {
                        // Expense: red when exceeded, warning when close
                        if (root.percentUsed > 100)
                            return Theme.negative;
                        if (root.percentUsed > 80)
                            return Theme.warning;
                        return Theme.positive;
                    }
                }
            }
        }

        Label {
            property double remaining: root.isIncome ? (root.budgetLimit - root.modelData.amount) : (root.modelData.amount - root.budgetLimit)
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
    }
}
