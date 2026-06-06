import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    title: qsTr("Create Counter Part")
    okEnabled: accountComboBox.currentIndex >= 0 && operation.account !== accountComboBox.currentAccount

    required property var operation

    signal createCounterPart(account: var, category: string)

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
            model: operation ? operation.allocatedCategoryNames : []
            enabled: allocationCheckBox.checked
        }
        AccountComboBox {
            id: accountComboBox
            Layout.fillWidth: true
            focus: true
        }
    }
}
