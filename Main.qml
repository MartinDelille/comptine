import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: budgetData.currentFilePath.length > 0 ? "Comptine - " + budgetData.currentFilePath.split('/').pop() : "Comptine"
    color: Theme.background

    property bool fileDialogOpen: openDialog.visible || saveDialog.visible || csvDialog.visible
    property string pendingAction: ""  // "quit", "new", or "open"
    property bool forceQuit: false  // Set to true when user confirmed quit without saving

    function performPendingAction() {
        if (pendingAction === "quit") {
            Qt.quit();
        } else if (pendingAction === "new") {
            budgetData.clear();
            budgetData.currentFilePath = "";
        } else if (pendingAction === "open") {
            openDialog.open();
        }
        pendingAction = "";
    }

    function checkUnsavedChanges(action) {
        if (!budgetData.undoStack.clean) {
            pendingAction = action;
            unsavedChangesDialog.open();
            return true;  // Has unsaved changes, action deferred
        }
        return false;  // No unsaved changes, proceed
    }

    onClosing: function (close) {
        if (!budgetData.undoStack.clean && !forceQuit) {
            close.accepted = false;
            pendingAction = "quit";
            unsavedChangesDialog.open();
        }
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&New...")
                shortcut: StandardKey.New
                onTriggered: {
                    if (!checkUnsavedChanges("new")) {
                        budgetData.clear();
                        budgetData.currentFilePath = "";
                    }
                }
            }
            Action {
                text: qsTr("&Open...")
                shortcut: StandardKey.Open
                onTriggered: {
                    if (!checkUnsavedChanges("open")) {
                        openDialog.open();
                    }
                }
            }
            Action {
                text: qsTr("&Save")
                shortcut: StandardKey.Save
                onTriggered: {
                    if (budgetData.currentFilePath.length > 0) {
                        budgetData.saveToYaml(budgetData.currentFilePath);
                    } else {
                        saveDialog.open();
                    }
                }
            }
            Action {
                text: qsTr("&Save As...")
                shortcut: StandardKey.SaveAs
                onTriggered: saveDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Import CSV...")
                shortcut: "Ctrl+Shift+I"
                onTriggered: csvDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Preferences...")
                shortcut: StandardKey.Preferences
                onTriggered: preferencesDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Quit")
                shortcut: StandardKey.Quit
                onTriggered: {
                    if (!checkUnsavedChanges("quit")) {
                        Qt.quit();
                    }
                }
            }
        }
        Menu {
            title: qsTr("&Edit")
            Action {
                text: qsTr("&Undo")
                shortcut: StandardKey.Undo
                enabled: budgetData.undoStack.canUndo
                onTriggered: budgetData.undo()
            }
            Action {
                text: qsTr("&Redo")
                shortcut: StandardKey.Redo
                enabled: budgetData.undoStack.canRedo
                onTriggered: budgetData.redo()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Copy")
                shortcut: StandardKey.Copy
                enabled: budgetData.operationModel.selectionCount > 0
                onTriggered: budgetData.copySelectedOperationsToClipboard()
            }
        }
        Menu {
            title: qsTr("&View")
            Action {
                text: qsTr("&About")
                onTriggered: aboutDialog.open()
            }
        }
        Menu {
            title: qsTr("&View")
            Action {
                text: qsTr("&Operations")
                shortcut: "Ctrl+1"
                onTriggered: budgetData.currentTabIndex = 0
            }
            Action {
                text: qsTr("&Budget")
                shortcut: "Ctrl+2"
                onTriggered: budgetData.currentTabIndex = 1
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Previous Month")
                shortcut: "Left"
                enabled: !fileDialogOpen
                onTriggered: {
                    if (budgetData.budgetMonth === 1) {
                        budgetData.budgetMonth = 12;
                        budgetData.budgetYear--;
                    } else {
                        budgetData.budgetMonth--;
                    }
                }
            }
            Action {
                text: qsTr("&Next Month")
                shortcut: "Right"
                enabled: !fileDialogOpen
                onTriggered: {
                    if (budgetData.budgetMonth === 12) {
                        budgetData.budgetMonth = 1;
                        budgetData.budgetYear++;
                    } else {
                        budgetData.budgetMonth++;
                    }
                }
            }
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Budget File")
        fileMode: FileDialog.OpenFile
        nameFilters: ["YAML files (*.yaml *.yml)", "All files (*)"]
        onAccepted: {
            budgetData.loadFromYaml(selectedFile.toString().replace("file://", ""));
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save Budget File")
        fileMode: FileDialog.SaveFile
        nameFilters: ["YAML files (*.yaml *.yml)", "All files (*)"]
        currentFile: budgetData.currentFilePath.length > 0 ? "file://" + budgetData.currentFilePath : ""
        onAccepted: {
            var filePath = selectedFile.toString().replace("file://", "");
            if (budgetData.saveToYaml(filePath)) {
                budgetData.currentFilePath = filePath;
                if (window.pendingAction !== "") {
                    window.performPendingAction();
                }
            }
        }
        onRejected: {
            window.pendingAction = "";
        }
    }

    FileDialog {
        id: csvDialog
        title: qsTr("Import CSV File")
        fileMode: FileDialog.OpenFile
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            importDialog.filePath = selectedFile.toString().replace("file://", "");
            importDialog.open();
        }
    }

    ImportDialog {
        id: importDialog
        anchors.centerIn: parent
    }

    Dialog {
        id: aboutDialog
        title: qsTr("About Comptine")
        standardButtons: Dialog.Ok

        Label {
            text: qsTr("Comptine v0.1\n\nPersonal Budget Management Software\n\nImport and manage your bank account data.")
        }
    }

    PreferencesDialog {
        id: preferencesDialog
        anchors.centerIn: parent
    }

    MessageDialog {
        id: unsavedChangesDialog
        title: qsTr("Unsaved Changes")
        text: qsTr("You have unsaved changes. Do you want to save before continuing?")
        buttons: MessageDialog.Save | MessageDialog.Discard | MessageDialog.Cancel
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Save) {
                if (budgetData.currentFilePath.length > 0) {
                    budgetData.saveToYaml(budgetData.currentFilePath);
                    window.performPendingAction();
                } else {
                    saveDialog.open();
                }
            } else if (button === MessageDialog.Discard) {
                forceQuit = (pendingAction === "quit");
                window.performPendingAction();
            } else {
                // Cancel: clear pending action
                window.pendingAction = "";
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingNormal

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: budgetData.currentTabIndex
            onCurrentIndexChanged: budgetData.currentTabIndex = currentIndex
            focusPolicy: Qt.NoFocus  // Prevent tab bar from stealing focus

            background: Rectangle {
                color: Theme.surface
            }

            TabButton {
                text: qsTr("Operations")
                focusPolicy: Qt.NoFocus
            }
            TabButton {
                text: qsTr("Budget")
                focusPolicy: Qt.NoFocus
            }
        }

        Connections {
            target: budgetData
            function onDataLoaded() {
                tabBar.currentIndex = budgetData.currentTabIndex;
            }
            function onCurrentTabIndexChanged() {
                tabBar.currentIndex = budgetData.currentTabIndex;
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Operations view
            OperationView {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Budget view
            BudgetView {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
