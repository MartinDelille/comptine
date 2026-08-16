pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import ui.common
import ui.budget
import ui.evolution
import ui.operations
import ui.rules
import services
import editor

ApplicationWindow {
    id: window
    x: AppSettings.windowX
    y: AppSettings.windowY
    width: AppSettings.windowWidth
    height: AppSettings.windowHeight
    visible: true
    title: FileController.currentFilePath.length > 0 ? "Comptine - " + FileController.currentFilePath.split('/').pop() : "Comptine"
    color: Theme.background

    property bool fileDialogOpen: openDialog.visible || saveDialog.visible || csvDialog.visible
    property bool anyDialogOpen: fileDialogOpen || importDialog.visible || aboutDialog.visible || preferencesDialog.visible || unsavedChangesDialog.visible || budgetView.dialogOpen || updateDialog.visible || rulesView.visible
    property string pendingAction: ""  // "quit", "new", or "open"
    property string pendingRecentFile: ""  // File path to open from recent files
    property bool forceQuit: false  // Set to true when user confirmed quit without saving

    Component.onDestruction: {
        AppSettings.windowX = x;
        AppSettings.windowY = y;
        AppSettings.windowWidth = width;
        AppSettings.windowHeight = height;
    }

    function performPendingAction() {
        if (pendingAction === "quit") {
            Qt.quit();
        } else if (pendingAction === "new") {
            FileController.clear();
        } else if (pendingAction === "open") {
            openDialog.open();
        } else if (pendingAction === "openRecent") {
            FileController.loadFromYamlFile(pendingRecentFile);
            pendingRecentFile = "";
        }
        pendingAction = "";
    }

    function checkUnsavedChanges(action) {
        if (FileController.hasUnsavedChanges) {
            pendingAction = action;
            unsavedChangesDialog.open();
            return true;  // Has unsaved changes, action deferred
        }
        return false;  // No unsaved changes, proceed
    }

    onClosing: function (close) {
        if (FileController.hasUnsavedChanges && !forceQuit) {
            close.accepted = false;
            pendingAction = "quit";
            unsavedChangesDialog.open();
        }
    }

    menuBar: ApplicationMenuBar {
        anyDialogOpen: window.anyDialogOpen
        window: window

        onNewFileAction: {
            if (!window.checkUnsavedChanges("new")) {
                FileController.clear();
            }
        }
        onOpenFileAction: {
            if (!window.checkUnsavedChanges("open")) {
                openDialog.open();
            }
        }
        onSaveFileDialogAction: saveDialog.open()
        onImportFileDialogAction: csvDialog.open()
        onOpenRecentFileAction: function (filePath) {
            if (!window.checkUnsavedChanges("openRecent")) {
                FileController.loadFromYamlFile(filePath);
            } else {
                window.pendingRecentFile = filePath;
            }
        }
        onQuitAction: {
            if (!window.checkUnsavedChanges("quit")) {
                Qt.quit();
            }
        }

        onAddAction: {
            if (BudgetData.currentTabIndex === 0) {
                operationView.addOperation();
            } else {
                budgetView.addCategory();
            }
        }
        onEditAction: {
            if (BudgetData.currentTabIndex === 0) {
                operationView.editCurrentOperation();
            } else {
                budgetView.editCurrentCategory();
            }
        }
        onDeleteAction: {
            if (BudgetData.currentTabIndex === 0) {
                deleteSelectedOperationsDialog.open();
            } else {
                deleteCurrentCagegoryDialog.open();
            }
        }
        onRulesAction: rulesView.open()
        onPreferencesAction: preferencesDialog.open()

        onCheckUpdateAction: {
            window.manualUpdateCheck = true;
            UpdateController.checkForUpdates();
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
            FileController.loadFromYamlUrl(selectedFile);
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save Budget File")
        fileMode: FileDialog.SaveFile
        nameFilters: ["Comptine files (*.comptine)", "All files (*)"]
        currentFile: FileController.currentFilePath.length > 0 ? "file://" + FileController.currentFilePath : ""
        onAccepted: {
            if (FileController.saveToYamlUrl(selectedFile)) {
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
            importDialog.fileUrls = selectedFiles;
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
    }

    BaseDialog {
        id: deleteSelectedOperationsDialog
        title: qsTr("Delete Operations")
        width: 400
        Label {
            text: qsTr("Are you sure you want to delete the selected operations?")
        }
        acceptButtonText: qsTr("Delete")
        onAccepted: OperationEditor.deleteSelected()
    }

    BaseDialog {
        id: deleteCurrentCagegoryDialog
        title: qsTr("Delete Category")
        width: 400
        Label {
            text: {
                var categoryName = CategoryController.current ? CategoryController.current.name : "";
                var count = BudgetData.countOperationsWithCategory(CategoryController.current);
                return (count > 0) ? qsTr(`Are you sure you want to delete ${categoryName} with ${count} allocations ?`) : qsTr(`Are you sure you want to delete ${categoryName} ?`);
            }
        }
        acceptButtonText: qsTr("Delete")
        onAccepted: CategoryEditor.remove(CategoryController.current)
    }

    RulesView {
        id: rulesView
    }

    UpdateDialog {
        id: updateDialog
    }

    MessageDialog {
        id: externalChangeDialog
        title: qsTr("File Changed Externally")
        text: qsTr("The current file has been modified outside of Comptine. Do you want to reload it? Any unsaved changes will be lost.")
        buttons: MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Yes) {
                FileController.loadFromYamlFile(FileController.currentFilePath);
            }
        }
    }

    MessageDialog {
        id: noUpdateDialog
        title: qsTr("No Update Available")
        text: qsTr("You are running the latest version of Comptine (%1).").arg(UpdateController.currentVersion())
        buttons: MessageDialog.Ok
    }

    MessageDialog {
        id: updateErrorDialog
        title: qsTr("Update Check Failed")
        text: UpdateController.errorMessage
        buttons: MessageDialog.Ok
    }

    // Track if update check was manual (user clicked menu) vs automatic
    property bool manualUpdateCheck: false

    // Auto-check for updates on startup
    Component.onCompleted: {
        if (UpdateController.shouldAutoCheck()) {
            UpdateController.checkForUpdates();
        }
    }

    MessageDialog {
        id: unsavedChangesDialog
        title: qsTr("Unsaved Changes")
        text: qsTr("You have unsaved changes. Do you want to save before continuing?")
        buttons: MessageDialog.Save | MessageDialog.Discard | MessageDialog.Cancel
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Save) {
                if (FileController.currentFilePath.length > 0) {
                    FileController.saveToYamlFile(FileController.currentFilePath);
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
        text: FileController.errorMessage
        buttons: MessageDialog.Ok
    }

    Connections {
        target: FileController
        function onErrorMessageChanged() {
            if (FileController.errorMessage.length > 0) {
                fileErrorDialog.open();
            }
        }
        function onExternalChangeDetected() {
            externalChangeDialog.open();
        }
    }

    // Handle update check results
    Connections {
        target: UpdateController
        function onUpdateCheckCompleted() {
            UpdateController.markUpdateChecked();
            if (UpdateController.updateAvailable) {
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingNormal
        spacing: Theme.spacingNormal

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: BudgetData.currentTabIndex
            onCurrentIndexChanged: BudgetData.currentTabIndex = currentIndex
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
            TabButton {
                text: qsTr("Evolution")
                focusPolicy: Qt.NoFocus
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Operations view
            OperationView {
                id: operationView
                focus: StackLayout.isCurrentItem
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            // Budget view
            BudgetView {
                id: budgetView
                focus: StackLayout.isCurrentItem
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            EvolutionView {
                focus: StackLayout.isCurrentItem
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
