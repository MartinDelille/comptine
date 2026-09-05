import QtQuick.Controls

import ui.common

Label {
    required property real amount
    text: Theme.formatAmount(amount)
    font.bold: true
    color: amount >= 0 ? Theme.positive : Theme.negative
}
