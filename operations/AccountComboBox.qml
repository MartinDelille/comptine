import QtQuick.Controls

ComboBox {
    required property var budgetData
    model: budgetData.accountModel
    enabled: budgetData.accountCount > 0
    textRole: "name"
    property var currentAccount: budgetData.accountModel.accountAt(currentIndex)
}
