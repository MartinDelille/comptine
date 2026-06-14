pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import Comptine
import budget
import commonui
import operations
import rules

ApplicationWindow {
    id: window
    x: AppState.settings.windowX
    y: AppState.settings.windowY
    width: AppState.settings.windowWidth
    height: AppState.settings.windowHeight
    visible: true
    title: AppState.file.currentFilePath.length > 0 ? "Comptine - " + AppState.file.currentFilePath.split('/').pop() : "Comptine"
    color: Theme.background

    property bool fileDialogOpen: openDialog.visible || saveDialog.visible || csvDialog.visible
    property bool anyDialogOpen: fileDialogOpen || importDialog.visible || aboutDialog.visible || preferencesDialog.visible || unsavedChangesDialog.visible || budgetView.dialogOpen || updateDialog.visible || rulesView.visible
    property string pendingAction: ""  // "quit", "new", or "open"
    property string pendingRecentFile: ""  // File path to open from recent files
    property bool forceQuit: false  // Set to true when user confirmed quit without saving

    Component.onDestruction: {
        AppState.settings.windowX = x;
        AppState.settings.windowY = y;
        AppState.settings.windowWidth = width;
        AppState.settings.windowHeight = height;
    }

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

    menuBar: ApplicationMenuBar {
        anyDialogOpen: window.anyDialogOpen
        onNewFileAction: {
            if (!window.checkUnsavedChanges("new")) {
                AppState.file.clear();
            }
        }
        onOpenFileAction: {
            if (!window.checkUnsavedChanges("open")) {
                openDialog.open();
            }
        }
        onSaveFileAction: saveAs => {
            if (!saveAs && AppState.file.currentFilePath.length > 0) {
                AppState.file.saveToYamlFile(AppState.file.currentFilePath);
            } else {
                saveDialog.open();
            }
        }
        onOpenRecentFileAction: filePath => {
            if (!window.checkUnsavedChanges("openRecent")) {
                AppState.file.loadFromYamlFile(filePath);
            } else {
                window.pendingRecentFile = filePath;
            }
        }
        onImportCsvAction: csvDialog.open()
        onQuitAction: {
            if (!window.checkUnsavedChanges("quit")) {
                Qt.quit();
            }
        }
        onAddAction: {
            if (AppState.navigation.currentTabIndex === 0) {
                operationView.addOperation();
            } else {
                budgetView.addCategory();
            }
        }
        onEditAction: {
            if (AppState.navigation.currentTabIndex === 0) {
                operationView.editCurrentOperation();
            } else {
                budgetView.editCurrentCategory();
            }
        }
        onDeleteAction: {
            if (AppState.navigation.currentTabIndex === 0) {
                deleteSelectedOperationsDialog.open();
            } else {
                deleteCurrentCagegoryDialog.open();
            }
        }
        onRulesAction: rulesView.open()
        onPreferencesAction: preferencesDialog.open()
        onCheckUpdateAction: {
            window.manualUpdateCheck = true;
            AppState.update.checkForUpdates();
        }
        onProjectPageAction: Qt.openUrlExternally("https://martin.delille.org/comptine/")
        onAboutAction: aboutDialog.open()
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
        title: qsTr("Import CSV Files")
        fileMode: FileDialog.OpenFiles
        nameFilters: ["CSV files (*.csv)", "All files (*)"]
        onAccepted: {
            importDialog.filePaths = selectedFiles;
            importDialog.open();
        }
    }

    ImportDialog {
        id: importDialog
    }

    AboutDialog {
        id: aboutDialog
    }

    PreferencesDialog {
        id: preferencesDialog
        settings: AppState.settings
    }

    BaseDialog {
        id: deleteSelectedOperationsDialog
        title: qsTr("Delete Operations")
        width: 400
        Label {
            text: qsTr("Are you sure you want to delete the selected operations?")
        }
        acceptButtonText: qsTr("Delete")
        onAccepted: AppState.data.deleteSelectedOperations()
    }

    BaseDialog {
        id: deleteCurrentCagegoryDialog
        title: qsTr("Delete Category")
        width: 400
        Label {
            property string _categoryName: AppState.categories.current ? AppState.categories.current.name : ""
            text: qsTr(`Are you sure you want to delete ${_categoryName} ?`)
        }
        acceptButtonText: qsTr("Delete")
        onAccepted: AppState.categories.deleteCategory(AppState.categories.current)
    }

    RulesView {
        id: rulesView
        categories: AppState.categories
        rules: AppState.rules
    }

    UpdateDialog {
        id: updateDialog
        update: AppState.update
    }

    MessageDialog {
        id: externalChangeDialog
        title: qsTr("File Changed Externally")
        text: qsTr("The current file has been modified outside of Comptine. Do you want to reload it? Any unsaved changes will be lost.")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Yes) {
                AppState.file.loadFromYamlFile(AppState.file.currentFilePath);
            }
        }
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
                window.forceQuit = (window.pendingAction === "quit");
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
        function onExternalChangeDetected() {
            externalChangeDialog.open();
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
                budgetData: AppState.data
                categories: AppState.categories
                navigation: AppState.navigation
                rules: AppState.rules
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Budget view
            BudgetView {
                id: budgetView
                categories: AppState.categories
                navigation: AppState.navigation
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
