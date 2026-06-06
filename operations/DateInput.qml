pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

Item {
    id: root
    property date selectedDate: new Date()
    width: textField.width
    height: textField.height
    TextField {
        id: textField
        text: Qt.formatDate(root.selectedDate, "dd/MM/yyyy")
        onPressed: popup.doSelectDate(root.selectedDate)
        onActiveFocusChanged: {
            if (activeFocus) {
                popup.doSelectDate(root.selectedDate);
            } else {
                popup.close();
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
                    root.selectedDate = date;
                    popup.close();
                }
                delegate: Rectangle {
                    id: dayDelegate
                    required property var model
                    color: {
                        if (hoverHandler.hovered) {
                            return Theme.backgroundHover;
                        }
                        if (model.day === root.selectedDate.getDate() && model.month === root.selectedDate.getMonth() && model.year === root.selectedDate.getFullYear()) {
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
                        opacity: dayDelegate.model.month === monthGrid.month ? 1 : 0
                        text: monthGrid.locale.toString(dayDelegate.model.date, "d")
                        font: monthGrid.font
                    }
                }
            }
        }
    }
}
