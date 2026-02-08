import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property date selectedDate: new Date()
    width: textField.width
    height: textField.height
    TextField {
        id: textField
        text: Qt.formatDate(selectedDate, "dd/MM/yyyy")
        onPressed: popup.doSelectDate(selectedDate)
        onFocusChanged: {
            if (focus) {
                popup.doSelectDate(selectedDate);
            }
        }

        horizontalAlignment: Text.AlignHCenter
    }
    Popup {
        id: popup
        x: textField.x
        y: textField.y + textField.height + 5
        width: 200
        height: 200

        function doSelectDate(date) {
            monthGrid.month = date.getMonth();
            monthGrid.year = date.getFullYear();
            open();
        }
        ColumnLayout {

            anchors.fill: parent
            RowLayout {
                id: headerRow
                Layout.fillWidth: true
                ToolButton {
                    text: "<"
                    font.weight: Font.Bold
                    onClicked: {
                        if (monthGrid.month > 0) {
                            monthGrid.month -= 1;
                        } else {
                            monthGrid.month = 11;
                            monthGrid.year -= 1;
                        }
                    }
                }
                Label {
                    text: monthGrid.locale.toString(new Date(monthGrid.year, monthGrid.month), "MMMM yyyy")
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: ">"
                    font.weight: Font.Bold
                    onClicked: {
                        if (monthGrid.month < 11) {
                            monthGrid.month += 1;
                        } else {
                            monthGrid.month = 0;
                            monthGrid.year += 1;
                        }
                    }
                }
            }
            DayOfWeekRow {
                id: dayOfWeekRow
                Layout.fillWidth: true
            }
            MonthGrid {
                id: monthGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                month: Calendar.December
                year: 2015
                locale: Qt.locale("en_US")

                onClicked: function (date) {
                    selectedDate = date;
                    popup.close();
                }
                delegate: Rectangle {
                    required property var model
                    color: {
                        if (hoverHandler.hovered) {
                            return Theme.backgroundHover;
                        }
                        if (model.day === selectedDate.getDate() && model.month === selectedDate.getMonth() && model.year === selectedDate.getFullYear()) {
                            return Theme.backgroundSelected;
                        }
                        return "transparent";
                    }
                    HoverHandler {
                        id: hoverHandler
                    }
                    Label {
                        anchors.fill: parent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        opacity: model.month === monthGrid.month ? 1 : 0
                        text: monthGrid.locale.toString(model.date, "d")
                        font: monthGrid.font
                    }
                }
            }
        }
    }
}
