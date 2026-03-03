import QtQuick.Controls
import Comptine

ComboBox {
    model: AppState.data.accountModel
    enabled: AppState.data.accountCount > 0
    textRole: "name"
    property var currentAccount: AppState.data.accountModel.accountAt(currentIndex)
}
