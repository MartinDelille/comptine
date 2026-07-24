import QtQuick.Controls

ComboBox {
    required property var budgetData
    model: budgetData
    enabled: budgetData.accountCount > 0
    textRole: "name"
    property var currentAccount: budgetData.accountAt(currentIndex)
}
