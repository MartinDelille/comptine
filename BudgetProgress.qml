import QtQuick

Rectangle {
    required property double value
    required property double total
    width: Math.min(value / total, 1.0) * parent.width
    height: parent.height
    radius: 4
}
