import QtQuick
import QtQuick.Layouts

import ui.common

Item {
    id: root

    required property double value
    required property double total
    property double startValue: 0
    property double scaleMaximum: Math.abs(total)
    property color fillColor: Theme.accent
    property real minimumSegmentWidth: 2
    property bool showBackground: true
    property bool showFill: true
    property bool showMarker: true

    readonly property bool hasLimit: Math.abs(total) > 0.0001
    readonly property bool hasScale: scaleMaximum > 0.0001
    readonly property double startRatio: hasScale ? Math.min(Math.max(Math.abs(startValue) / scaleMaximum, 0), 1) : 0
    readonly property double endRatio: hasScale ? Math.min(Math.max(Math.abs(value) / scaleMaximum, 0), 1) : 0
    readonly property double budgetRatio: hasScale ? Math.min(Math.abs(total) / scaleMaximum, 1) : 0

    Layout.fillWidth: true
    Layout.preferredHeight: 12
    implicitHeight: 12

    Rectangle {
        anchors.fill: parent
        color: Theme.progressBackground
        radius: height / 2
        visible: root.showBackground
    }

    Item {
        id: fillSegment
        x: root.startRatio * parent.width
        width: root.hasScale ? Math.min(parent.width - x, Math.max((root.endRatio - root.startRatio) * parent.width, root.minimumSegmentWidth)) : 0
        height: parent.height
        clip: true
        visible: root.showFill && root.hasLimit && root.endRatio > root.startRatio

        Rectangle {
            anchors.fill: parent
            color: root.fillColor
            radius: 0
        }
    }

    Rectangle {
        x: root.budgetRatio * root.width - width / 2
        width: 2
        height: parent.height
        color: Theme.textPrimary
        visible: root.hasLimit && root.hasScale && root.showMarker
    }
}
