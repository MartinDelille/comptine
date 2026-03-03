pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Comptine

BaseDialog {
    id: root
    title: qsTr("Categorization Rules")
    width: 600
    height: 500
    rejectButtonText: ""

    RuleEditDialog {
        id: ruleEditDialog
        onClosed: ruleListView.forceActiveFocus()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        // Header with label and Add button
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                Layout.fillWidth: true
                text: qsTr("Rules are matched in order. The first matching rule wins.")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }

            Button {
                text: qsTr("Add Rule...")
                onClicked: {
                    ruleEditDialog.isNewRule = true;
                    ruleEditDialog.suggestedPrefix = "";
                    ruleEditDialog.open();
                }
            }
        }

        // Rules list
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.borderLight
            border.width: Theme.cardBorderWidth
            radius: Theme.cardRadius

            ListView {
                id: ruleListView
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall
                model: AppState.rules.ruleModel
                clip: true
                focus: true
                spacing: Theme.spacingSmall

                delegate: Rectangle {
                    id: ruleDelegate
                    required property int index
                    required property string category
                    required property string labelPrefix
                    required property double amountFilter

                    width: ListView.view.width
                    height: contentRow.implicitHeight + Theme.spacingNormal * 2
                    color: delegateMouseArea.containsMouse ? Theme.backgroundHover : Theme.surfaceElevated
                    border.color: ListView.isCurrentItem ? Theme.accent : Theme.borderLight
                    border.width: ListView.isCurrentItem ? 2 : Theme.cardBorderWidth
                    radius: Theme.cardRadius

                    MouseArea {
                        id: delegateMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: ruleListView.currentIndex = ruleDelegate.index
                        onDoubleClicked: {
                            ruleEditDialog.isNewRule = false;
                            ruleEditDialog.ruleIndex = ruleDelegate.index;
                            ruleEditDialog.originalCategory = ruleDelegate.category;
                            ruleEditDialog.originalLabelPrefix = ruleDelegate.labelPrefix;
                            ruleEditDialog.originalAmountFilter = ruleDelegate.amountFilter;
                            ruleEditDialog.open();
                        }
                    }

                    RowLayout {
                        id: contentRow
                        anchors.fill: parent
                        anchors.margins: Theme.spacingNormal
                        spacing: Theme.spacingNormal

                        // Priority indicator
                        Label {
                            text: (ruleDelegate.index + 1).toString()
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textMuted
                            Layout.preferredWidth: 24
                            horizontalAlignment: Text.AlignHCenter
                        }

                        // Rule info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                text: qsTr("Prefix: \"%1\"").arg(ruleDelegate.labelPrefix)
                                font.pixelSize: Theme.fontSizeNormal
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: ruleDelegate.amountFilter !== 0 ? qsTr("Assign to: %1 (amount: %2)").arg(ruleDelegate.category).arg(Theme.formatAmount(ruleDelegate.amountFilter)) : qsTr("Assign to: %1").arg(ruleDelegate.category)
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                            }
                        }

                        // Move up button
                        ToolButton {
                            text: "\u2191"
                            font.pixelSize: Theme.fontSizeNormal
                            enabled: ruleDelegate.index > 0
                            opacity: enabled ? (hovered ? 1.0 : 0.5) : 0.2
                            focusPolicy: Qt.NoFocus
                            onClicked: AppState.rules.moveRule(ruleDelegate.index, ruleDelegate.index - 1)
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Move up (higher priority)")
                        }

                        // Move down button
                        ToolButton {
                            text: "\u2193"
                            font.pixelSize: Theme.fontSizeNormal
                            enabled: ruleDelegate.index < AppState.rules.ruleCount - 1
                            opacity: enabled ? (hovered ? 1.0 : 0.5) : 0.2
                            focusPolicy: Qt.NoFocus
                            onClicked: AppState.rules.moveRule(ruleDelegate.index, ruleDelegate.index + 1)
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Move down (lower priority)")
                        }

                        // Edit button
                        ToolButton {
                            text: "\u270E"
                            font.pixelSize: Theme.fontSizeNormal
                            focusPolicy: Qt.NoFocus
                            opacity: hovered ? 1.0 : 0.5
                            onClicked: {
                                ruleEditDialog.isNewRule = false;
                                ruleEditDialog.ruleIndex = ruleDelegate.index;
                                ruleEditDialog.originalCategory = ruleDelegate.category;
                                ruleEditDialog.originalLabelPrefix = ruleDelegate.labelPrefix;
                                ruleEditDialog.originalAmountFilter = ruleDelegate.amountFilter;
                                ruleEditDialog.open();
                            }
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Edit rule")
                        }

                        // Delete button
                        ToolButton {
                            text: "\u2715"
                            font.pixelSize: Theme.fontSizeNormal
                            focusPolicy: Qt.NoFocus
                            opacity: hovered ? 1.0 : 0.5
                            onClicked: AppState.rules.removeRule(ruleDelegate.index)
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Delete rule")
                        }
                    }
                }
            }

            // Empty state
            Label {
                anchors.centerIn: parent
                visible: AppState.rules.ruleCount === 0
                text: qsTr("No rules defined.\nClick \"Add Rule...\" to create one.")
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeNormal
                color: Theme.textMuted
            }
        }

        // Footer with stats
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal

            Label {
                text: qsTr("%1 rule(s)").arg(AppState.rules.ruleCount)
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
        }
    }
}
