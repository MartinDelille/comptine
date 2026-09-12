pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common

Rectangle {
    id: root

    required property var categories
    required property var budgetData
    required property var modelData
    required property int index
    required property bool isCurrentItem
    required property var categoryEditor

    property var category: modelData.category
    property double amount: modelData.amount || 0
    property double budgetLimit: modelData.budgetLimit || 0
    property bool isIncome: budgetLimit > 0
    property double leftover: modelData.leftover || 0
    property double saveAmount: modelData.saveAmount || 0
    property double reportAmount: modelData.reportAmount || 0
    property double accumulated: modelData.accumulated || 0
    property bool expanded: isCurrentItem

    readonly property double remainingLeftover: leftover - saveAmount - reportAmount
    readonly property bool isBalanced: Math.abs(remainingLeftover) < 0.01
    readonly property double absoluteBudget: Math.abs(budgetLimit)
    readonly property double absoluteAmount: Math.abs(amount)
    readonly property double usageRatio: absoluteBudget > 0 ? absoluteAmount / absoluteBudget : 0
    readonly property double usagePercent: usageRatio * 100
    readonly property double remaining: isIncome ? budgetLimit - amount : amount - budgetLimit
    readonly property double positiveReportAmount: Math.max(reportAmount, 0)
    readonly property double negativeReportAmount: Math.abs(Math.min(reportAmount, 0))
    readonly property double savedAmount: Math.abs(saveAmount)
    readonly property double accountingTotal: absoluteAmount + positiveReportAmount + savedAmount
    readonly property double scaleMaximum: Math.max(absoluteBudget, accountingTotal)
    readonly property double reportSegmentStart: absoluteAmount
    readonly property double reportSegmentEnd: reportSegmentStart + positiveReportAmount
    readonly property double saveSegmentStart: reportSegmentEnd
    readonly property double saveSegmentEnd: saveSegmentStart + savedAmount
    readonly property double deficitSegmentStart: Math.max(0, accountingTotal - negativeReportAmount)
    readonly property double deficitSegmentEnd: accountingTotal

    signal clicked
    signal editClicked

    implicitHeight: contentColumn.implicitHeight + 24
    color: delegateMouseArea.containsMouse ? Theme.backgroundHover : Theme.surfaceElevated
    border.color: isCurrentItem ? Theme.accent : (isBalanced ? Theme.positive : Theme.borderLight)
    border.width: isCurrentItem ? 2 : Theme.cardBorderWidth
    radius: Theme.cardRadius

    function statusText() {
        if (absoluteBudget <= 0)
            return qsTr("No budget limit");

        if (isIncome) {
            return remaining > 0 ? qsTr("Expected: %1 · %2% received").arg(Theme.formatAmount(remaining)).arg(usagePercent.toFixed(0)) : qsTr("Received: %1 extra · %2% received").arg(Theme.formatAmount(-remaining)).arg(usagePercent.toFixed(0));
        }
        return remaining >= 0 ? qsTr("Remaining: %1 · %2% used").arg(Theme.formatAmount(remaining)).arg(usagePercent.toFixed(0)) : qsTr("Exceeded by %1 · %2% used").arg(Theme.formatAmount(-remaining)).arg(usagePercent.toFixed(0));
    }

    function statusColor() {
        if (isIncome)
            return remaining > 0 ? Theme.warning : Theme.positive;
        return remaining >= 0 ? Theme.textSecondary : Theme.negative;
    }

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

        ColumnLayout {
            id: collapsedColumn
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Label {
                    text: root.expanded ? "⌄" : "›"
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeLarge
                    Layout.preferredWidth: 14
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    text: root.category?.name || ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Label {
                    text: root.isIncome ? qsTr("Income") : qsTr("Expense")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeSmall
                }

                Label {
                    text: Theme.formatAmount(root.absoluteAmount) + " / " + Theme.formatAmount(root.absoluteBudget)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeNormal
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: 170
                }

                Label {
                    text: root.isBalanced ? qsTr("Balanced") : qsTr("Needs attention")
                    color: root.isBalanced ? Theme.positive : Theme.warning
                    font.pixelSize: Theme.fontSizeSmall
                    font.weight: Font.Medium
                    horizontalAlignment: Text.AlignRight
                    Layout.preferredWidth: 110
                }

                ToolButton {
                    text: "✏"
                    font.pixelSize: Theme.fontSizeSmall
                    padding: 0
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    Layout.minimumWidth: 28
                    Layout.minimumHeight: 28
                    focusPolicy: Qt.TabFocus
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Edit category")
                    onClicked: root.editClicked()
                }
            }

            Item {
                id: allocationProgressBar
                Layout.fillWidth: true
                Layout.preferredHeight: 12

                BudgetProgress {
                    anchors.fill: parent
                    value: 0
                    total: root.absoluteBudget
                    scaleMaximum: root.scaleMaximum
                    showMarker: false
                    showFill: false
                }

                BudgetProgress {
                    anchors.fill: parent
                    value: root.absoluteAmount
                    total: root.absoluteBudget
                    scaleMaximum: root.scaleMaximum
                    fillColor: root.negativeReportAmount > 0.0001 ? Theme.accent : (root.isIncome ? Theme.positive : (root.remaining < 0 ? Theme.negative : Theme.accent))
                    showBackground: false
                    showMarker: false
                }

                BudgetProgress {
                    anchors.fill: parent
                    startValue: root.reportSegmentStart
                    value: root.reportSegmentEnd
                    total: root.absoluteBudget
                    scaleMaximum: root.scaleMaximum
                    fillColor: Theme.warning
                    showBackground: false
                    showMarker: false
                    visible: root.positiveReportAmount > 0.0001
                }

                BudgetProgress {
                    anchors.fill: parent
                    startValue: root.saveSegmentStart
                    value: root.saveSegmentEnd
                    total: root.absoluteBudget
                    scaleMaximum: root.scaleMaximum
                    fillColor: root.isIncome ? Theme.accent : Theme.positive
                    showBackground: false
                    showMarker: false
                    visible: root.savedAmount > 0.0001
                }

                BudgetProgress {
                    anchors.fill: parent
                    startValue: root.deficitSegmentStart
                    value: root.deficitSegmentEnd
                    total: root.absoluteBudget
                    scaleMaximum: root.scaleMaximum
                    fillColor: Theme.negative
                    showBackground: false
                    showMarker: false
                    visible: root.negativeReportAmount > 0.0001
                }

                BudgetProgress {
                    anchors.fill: parent
                    value: 0
                    total: root.absoluteBudget
                    scaleMaximum: root.scaleMaximum
                    showBackground: false
                    showFill: false
                }
            }

            Label {
                text: root.statusText()
                color: root.statusColor()
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignRight
                Layout.fillWidth: true
            }
        }

        ColumnLayout {
            id: expandedColumn
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            visible: root.expanded

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderLight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingNormal

                Label {
                    text: qsTr("Save")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                }

                AmountField {
                    id: saveField
                    Layout.preferredWidth: 72
                    Layout.maximumWidth: 90
                    Layout.preferredHeight: 28
                    value: root.saveAmount
                    onEdited: newValue => root.categoryEditor.setSaveAmount(root.category, root.budgetData.budgetDate, newValue)
                }

                ToolButton {
                    property bool canDiscard: root.saveAmount !== 0
                    text: canDiscard ? "✖" : "⬆"
                    font.pixelSize: Theme.fontSizeSmall
                    padding: 0
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    Layout.minimumWidth: 28
                    Layout.minimumHeight: 28
                    enabled: !(root.isBalanced && root.saveAmount === 0)
                    ToolTip.visible: hovered
                    ToolTip.text: canDiscard ? qsTr("Clear saved amount") : qsTr("Allocate remaining to Save")
                    onClicked: {
                        if (canDiscard)
                            root.categoryEditor.setSaveAmount(root.category, root.budgetData.budgetDate, 0);
                        else
                            root.categoryEditor.setSaveAmount(root.category, root.budgetData.budgetDate, root.saveAmount + root.remainingLeftover);
                    }
                }

                Label {
                    text: qsTr("Report")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                }

                AmountField {
                    id: reportField
                    Layout.preferredWidth: 72
                    Layout.maximumWidth: 90
                    Layout.preferredHeight: 28
                    value: root.reportAmount
                    onEdited: newValue => root.categoryEditor.setReportAmount(root.category, root.budgetData.budgetDate, newValue)
                }

                ToolButton {
                    property bool canDiscard: root.reportAmount !== 0
                    text: canDiscard ? "✖" : "⬆"
                    font.pixelSize: Theme.fontSizeSmall
                    padding: 0
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    Layout.minimumWidth: 28
                    Layout.minimumHeight: 28
                    enabled: !(root.isBalanced && root.reportAmount === 0)
                    ToolTip.visible: hovered
                    ToolTip.text: canDiscard ? qsTr("Clear reported amount") : qsTr("Allocate remaining to Report")
                    onClicked: {
                        if (canDiscard) {
                            root.categoryEditor.setReportAmount(root.category, root.budgetData.budgetDate, 0);
                        } else if (root.leftover >= 0) {
                            root.categoryEditor.setReportAmount(root.category, root.budgetData.budgetDate, root.reportAmount + root.remainingLeftover);
                        } else {
                            root.categoryEditor.setReportAmount(root.category, root.budgetData.budgetDate, root.leftover);
                        }
                    }
                }
                Item {
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Accumulated leftover")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                }

                AmountLabel {
                    amount: root.accumulated
                    font.pixelSize: Theme.fontSizeSmall
                    Layout.preferredWidth: 80
                }

                Label {
                    text: qsTr("Available")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                }

                AmountLabel {
                    amount: root.leftover
                    font.pixelSize: Theme.fontSizeSmall
                    Layout.preferredWidth: 80
                }
            }
        }
    }
}
