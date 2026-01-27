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

    signal clicked
    signal editClicked

    implicitHeight: contentColumn.implicitHeight + 24
    color: delegateMouseArea.containsMouse ? Theme.surface : Theme.surfaceElevated
    border.color: isCurrentItem ? Theme.accent : Theme.borderLight
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
