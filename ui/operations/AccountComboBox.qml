import QtQuick.Controls

import services

ComboBox {
    model: BudgetData
    enabled: BudgetData.accountCount > 0
    textRole: "name"
    property var currentAccount: BudgetData.at(currentIndex)
}
