import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: AppState.file.currentFilePath.length > 0 ? "Comptine - " + AppState.file.currentFilePath.split('/').pop() : "Comptine"
    color: Theme.background

    property bool fileDialogOpen: openDialog.visible || saveDialog.visible || csvDialog.visible
    property bool anyDialogOpen: fileDialogOpen || importDialog.visible || aboutDialog.visible || preferencesDialog.visible || unsavedChangesDialog.visible || budgetView.dialogOpen || updateDialog.visible || categorizeDialog.visible || rulesView.visible
    property string pendingAction: ""  // "quit", "new", or "open"
    property string pendingRecentFile: ""  // File path to open from recent files
    property bool forceQuit: false  // Set to true when user confirmed quit without saving

    function performPendingAction() {
        if (pendingAction === "quit") {
            Qt.quit();
        } else if (pendingAction === "new") {
            AppState.file.clear();
        } else if (pendingAction === "open") {
            openDialog.open();
        } else if (pendingAction === "openRecent") {
            AppState.file.loadFromYamlFile(pendingRecentFile);
            pendingRecentFile = "";
        }
        pendingAction = "";
    }

    function openRecentFile(filePath) {
        if (checkUnsavedChanges("openRecent")) {
            pendingRecentFile = filePath;
        } else {
            AppState.file.loadFromYamlFile(filePath);
        }
    }

    function checkUnsavedChanges(action) {
        if (AppState.file.hasUnsavedChanges) {
            pendingAction = action;
            unsavedChangesDialog.open();
            return true;  // Has unsaved changes, action deferred
        }
        return false;  // No unsaved changes, proceed
    }

    onClosing: function (close) {
        if (AppState.file.hasUnsavedChanges && !forceQuit) {
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
                        AppState.file.clear();
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
                    if (AppState.file.currentFilePath.length > 0) {
                        AppState.file.saveToYamlFile(AppState.file.currentFilePath);
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
            Menu {
                id: recentFilesMenu
                title: qsTr("Open &Recent")
                enabled: AppState.settings.recentFilesModel.rowCount() > 0

                Instantiator {
                    model: AppState.settings.recentFilesModel
                    delegate: MenuItem {
                        text: model.display
                        onTriggered: window.openRecentFile(model.display)
                    }
                    onObjectAdded: (index, object) => recentFilesMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => recentFilesMenu.removeItem(object)
                }

                MenuSeparator {
                    visible: AppState.settings.recentFilesModel.rowCount() > 0
                }

                MenuItem {
                    text: qsTr("Clear Recent Files")
                    enabled: AppState.settings.recentFilesModel.rowCount() > 0
                    onTriggered: AppState.settings.clearRecentFiles()
                }
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Import CSV...")
                shortcut: "Ctrl+Shift+I"
                onTriggered: csvDialog.open()
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
                enabled: AppState.data.undoStack.canUndo
                onTriggered: AppState.data.undo()
            }
            Action {
                text: qsTr("&Redo")
                shortcut: StandardKey.Redo
                enabled: AppState.data.undoStack.canRedo
                onTriggered: AppState.data.redo()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Copy")
                shortcut: StandardKey.Copy
                enabled: AppState.data.operationModel.selectionCount > 0
                onTriggered: AppState.clipboard.copySelectedOperations()
            }
            MenuSeparator {}
            Action {
                text: qsTr("Add New Category...")
                shortcut: "Ctrl+Shift+N"
                enabled: AppState.navigation.currentTabIndex === 1
                onTriggered: {
                    budgetView.addCategory();
                }
            }
            Action {
                text: AppState.navigation.currentTabIndex === 0 ? qsTr("Edit &Operation...") : qsTr("Edit &Category...")
                shortcut: "Ctrl+E"
                enabled: (AppState.navigation.currentTabIndex === 0 && AppState.data.operationModel.selectionCount === 1) || (AppState.navigation.currentTabIndex === 1 && AppState.navigation.currentCategoryIndex >= 0)
                onTriggered: {
                    if (AppState.navigation.currentTabIndex === 0) {
                        operationView.editCurrentOperation();
                    } else {
                        budgetView.editCurrentCategory();
                    }
                }
            }
            MenuSeparator {}
            MenuSeparator {}
            Action {
                text: qsTr("&Categorize...")
                shortcut: "Ctrl+Shift+C"
                onTriggered: categorizeDialog.open()
            }
            Action {
                text: qsTr("Categorization &Rules...")
                onTriggered: rulesView.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Preferences...")
                shortcut: StandardKey.Preferences
                onTriggered: preferencesDialog.open()
            }
        }
        Menu {
            title: qsTr("&View")
            Action {
                text: qsTr("&Operations")
                shortcut: "Ctrl+1"
                onTriggered: AppState.navigation.showOperationsTab()
            }
            Action {
                text: qsTr("&Budget")
                shortcut: "Ctrl+2"
                onTriggered: AppState.navigation.showBudgetTab()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Previous Month")
                shortcut: "Left"
                enabled: AppState.navigation.currentTabIndex === 1 && !anyDialogOpen
                onTriggered: AppState.navigation.previousMonth()
            }
            Action {
                text: qsTr("&Next Month")
                shortcut: "Right"
                enabled: AppState.navigation.currentTabIndex === 1 && !anyDialogOpen
                onTriggered: AppState.navigation.nextMonth()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("Check for &Updates...")
                onTriggered: {
                    window.manualUpdateCheck = true;
                    AppState.update.checkForUpdates();
                }
            }
            Action {
                text: qsTr("&Project Page")
                onTriggered: Qt.openUrlExternally("https://martin.delille.org/comptine/")
            }
            MenuSeparator {}
            Action {
                text: qsTr("&About Comptine")
                onTriggered: aboutDialog.open()
            }
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Budget File")
        fileMode: FileDialog.OpenFile
        nameFilters: ["Comptine files (*.comptine)", "All files (*)"]
        onAccepted: {
            AppState.file.loadFromYamlUrl(selectedFile);
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save Budget File")
        fileMode: FileDialog.SaveFile
        nameFilters: ["Comptine files (*.comptine)", "All files (*)"]
        currentFile: AppState.file.currentFilePath.length > 0 ? "file://" + AppState.file.currentFilePath : ""
        onAccepted: {
            if (AppState.file.saveToYamlUrl(selectedFile)) {
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
            importDialog.filePath = selectedFile;
            importDialog.open();
        }
    }

    ImportDialog {
        id: importDialog
        anchors.centerIn: parent
    }

    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
    }

    PreferencesDialog {
        id: preferencesDialog
        anchors.centerIn: parent
    }

    CategorizeDialog {
        id: categorizeDialog
        anchors.centerIn: parent
    }

    RulesView {
        id: rulesView
        anchors.centerIn: parent
    }

    UpdateDialog {
        id: updateDialog
        anchors.centerIn: parent
    }

    MessageDialog {
        id: noUpdateDialog
        title: qsTr("No Update Available")
        text: qsTr("You are running the latest version of Comptine (%1).").arg(AppState.update.currentVersion())
        buttons: MessageDialog.Ok
    }

    MessageDialog {
        id: updateErrorDialog
        title: qsTr("Update Check Failed")
        text: AppState.update.errorMessage
        buttons: MessageDialog.Ok
    }

    // Track if update check was manual (user clicked menu) vs automatic
    property bool manualUpdateCheck: false

    // Handle update check results
    Connections {
        target: AppState.update
        function onUpdateCheckCompleted() {
            AppState.update.markUpdateChecked();
            if (AppState.update.updateAvailable) {
                updateDialog.open();
            } else if (window.manualUpdateCheck) {
                noUpdateDialog.open();
            }
            window.manualUpdateCheck = false;
        }
        function onUpdateCheckFailed(error) {
            if (window.manualUpdateCheck) {
                updateErrorDialog.open();
            }
            window.manualUpdateCheck = false;
        }
    }

    // Auto-check for updates on startup
    Component.onCompleted: {
        if (AppState.update.shouldAutoCheck()) {
            AppState.update.checkForUpdates();
        }
    }

    MessageDialog {
        id: unsavedChangesDialog
        title: qsTr("Unsaved Changes")
        text: qsTr("You have unsaved changes. Do you want to save before continuing?")
        buttons: MessageDialog.Save | MessageDialog.Discard | MessageDialog.Cancel
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Save) {
                if (AppState.file.currentFilePath.length > 0) {
                    AppState.file.saveToYamlFile(AppState.file.currentFilePath);
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

    MessageDialog {
        id: fileErrorDialog
        title: qsTr("File Error")
        text: AppState.file.errorMessage
        buttons: MessageDialog.Ok
    }

    Connections {
        target: AppState.file
        function onErrorMessageChanged() {
            if (AppState.file.errorMessage.length > 0) {
                fileErrorDialog.open();
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
            currentIndex: AppState.navigation.currentTabIndex
            onCurrentIndexChanged: AppState.navigation.currentTabIndex = currentIndex
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
            target: AppState.file
            function onDataLoaded() {
                // Focus the appropriate view after data load
                if (AppState.navigation.currentTabIndex === 0) {
                    operationView.forceActiveFocus();
                } else {
                    budgetView.forceActiveFocus();
                }
            }
        }

        Connections {
            target: AppState.navigation
            function onCurrentTabIndexChanged() {
                // Focus the appropriate view when tab changes
                if (AppState.navigation.currentTabIndex === 0) {
                    operationView.forceActiveFocus();
                } else {
                    budgetView.forceActiveFocus();
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Operations view
            OperationView {
                id: operationView
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Budget view
            BudgetView {
                id: budgetView
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
