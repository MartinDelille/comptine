import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    required property var updater

    title: qsTr("Comptine Update")
    width: 460
    height: 150
    minimumWidth: 460
    maximumWidth: 460
    minimumHeight: 150
    maximumHeight: 150
    visible: true
    flags: Qt.Dialog | Qt.CustomizeWindowHint | Qt.WindowTitleHint | (Qt.platform.os === "windows" ? Qt.WindowStaysOnTopHint : 0)

    function activateUpdater() {
        root.show();
        root.raise();
        root.requestActivate();
    }

    onClosing: function (close) {
        close.accepted = updater.finished || updater.failed;
    }

    Component.onCompleted: {
        root.activateUpdater();
    }

    onVisibleChanged: {
        if (visible)
            root.activateUpdater();
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            Layout.fillWidth: true
            text: root.updater.failed ? root.updater.errorMessage : root.updater.statusText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: root.updater.indeterminate
            value: root.updater.progress
            visible: !root.updater.failed
        }
    }
}
