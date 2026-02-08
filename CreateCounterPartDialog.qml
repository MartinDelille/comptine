import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

BaseDialog {
    title: qsTr("Create Counter Part")
    okEnabled: accountComboBox.currentIndex >= 0 && operation.account !== accountComboBox.currentAccount

    required property var operation

    signal createCounterPart(account: var)

    onAccepted: {
        createCounterPart(accountComboBox.currentAccount);
    }
    RowLayout {
        anchors.fill: parent

        Label {
            text: qsTr("Account:")
        }
        AccountComboBox {
            id: accountComboBox
            Layout.fillWidth: true
            focus: true
        }
    }
}
