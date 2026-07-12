import QtQuick.Controls

import commonui

Label {
    required property real amount
    text: Theme.formatAmount(amount)
    font.bold: true
    color: amount >= 0 ? Theme.positive : Theme.negative
}
