import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

BaseDialog {
    id: root
    title: qsTr("Create Counter Part")

    required property var budgetData
    required property var operation

    signal createCounterPart(account: var, category: string)

    okEnabled: accountComboBox.currentIndex >= 0 && operation.account !== accountComboBox.currentAccount

    onAccepted: {
        createCounterPart(accountComboBox.currentAccount, allocationCheckBox.checked ? allocationComboBox.currentText : "");
    }
    RowLayout {
        anchors.fill: parent

        Label {
            text: qsTr("Account:")
        }
        CheckBox {
            id: allocationCheckBox
            text: qsTr("On one allocation")
        }
        ComboBox {
            id: allocationComboBox
            model: root.operation ? root.operation.allocatedCategoryNames : []
            enabled: allocationCheckBox.checked
        }
        AccountComboBox {
            id: accountComboBox
            budgetData: root.budgetData
            Layout.fillWidth: true
            focus: true
        }
    }
}
