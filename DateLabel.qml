import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Label {
    required property var date
    function monthName(month) {
        var months = [qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"), qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"), qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")];
        return months[month - 1];
    }

    text: monthName(date.getMonth() + 1) + " " + date.getFullYear()
    font.pixelSize: Theme.fontSizeXLarge
    font.bold: true
}
