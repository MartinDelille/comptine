import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The updater singleton is registered by UpdaterForeigners.h in this module.
// The linter cannot resolve same-module C++ singleton references from this QML file.
// qmllint disable unqualified

ApplicationWindow {
    id: root

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
        close.accepted = UpdaterController.finished || UpdaterController.failed;
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
            text: UpdaterController.failed ? UpdaterController.errorMessage : UpdaterController.statusText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ProgressBar {
            Layout.fillWidth: true
            indeterminate: UpdaterController.indeterminate
            value: UpdaterController.progress
            visible: !UpdaterController.failed
        }
    }
}
// qmllint enable unqualified
