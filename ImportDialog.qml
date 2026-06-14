pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Comptine
import commonui
import operations

BaseDialog {
    id: importDialog
    title: qsTr("Import CSV Files")
    acceptButtonText: qsTr("Import All")
    width: 600

    property var filePaths: []

    // Model built from filePaths with per-file account info
    ListModel {
        id: fileEntries
    }

    okEnabled: fileEntries.count > 0

    onOpened: {
        fileEntries.clear();
        for (var i = 0; i < filePaths.length; i++) {
            var url = filePaths[i].toString();
            var fileName = url.substring(url.lastIndexOf("/") + 1);
            // Remove query/fragment if present
            var qIdx = fileName.indexOf("?");
            if (qIdx >= 0)
                fileName = fileName.substring(0, qIdx);
            fileName = decodeURIComponent(fileName);

            // Try auto-suggestion based on stored import sources
            var suggested = AppState.data.suggestedAccountForFile(fileName);
            var isNew = true;
            var existingIndex = -1;

            if (suggested !== "") {
                // Check if the suggested account exists
                var account = AppState.data.accountByName(suggested);
                if (account) {
                    isNew = false;
                    existingIndex = AppState.data.accountIndex(account);
                }
            } else {
                // Fall back to filename without extension
                var dotIdx = fileName.lastIndexOf(".");
                suggested = dotIdx > 0 ? fileName.substring(0, dotIdx) : fileName;
                // Check if this name matches an existing account
                var existing = AppState.data.accountByName(suggested);
                if (existing) {
                    isNew = false;
                    existingIndex = AppState.data.accountIndex(existing);
                }
            }

            fileEntries.append({
                url: filePaths[i],
                fileName: fileName,
                accountName: suggested,
                isNewAccount: isNew,
                existingAccountIndex: existingIndex >= 0 ? existingIndex : 0
            });
        }
        useCategoriesCheckBox.checked = false;
    }

    onAccepted: {
        for (var i = 0; i < fileEntries.count; i++) {
            var entry = fileEntries.get(i);
            AppState.file.importFromCsv(entry.url, entry.accountName.trim(), useCategoriesCheckBox.checked);
        }
        AppState.navigation.currentTabIndex = 0;
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingNormal

        Label {
            text: qsTr("Assign each file to an account:")
            font.pixelSize: Theme.fontSizeNormal
        }

        ListView {
            id: fileListView
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 300)
            clip: true
            model: fileEntries
            spacing: Theme.spacingNormal

            delegate: ColumnLayout {
                id: fileDelegate
                required property var model
                required property int index
                width: parent.width
                spacing: Theme.spacingSmall

                RowLayout {
                    Label {
                        text: fileDelegate.model.fileName
                        font.pixelSize: Theme.fontSizeNormal
                        font.bold: true
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }

                    ToolButton {
                        text: "🗑️"
                        onClicked: {
                            if (fileDelegate.index < fileEntries.count) {
                                fileEntries.remove(fileDelegate.index);
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: Theme.spacingNormal
                    Layout.fillWidth: true

                    CheckBox {
                        id: newAccountCheck
                        text: qsTr("New account")
                        checked: fileDelegate.model.isNewAccount

                        onCheckedChanged: {
                            fileDelegate.model.isNewAccount = checked;
                        }
                    }

                    AccountComboBox {
                        id: accountCombo
                        budgetData: AppState.data
                        Layout.fillWidth: true
                        visible: !newAccountCheck.checked
                        currentIndex: fileDelegate.model.existingAccountIndex

                        onCurrentIndexChanged: fileDelegate.model.existingAccountIndex = currentIndex
                    }

                    TextField {
                        id: newAccountField
                        Layout.fillWidth: true
                        visible: newAccountCheck.checked
                        placeholderText: qsTr("Account name")
                        text: fileDelegate.model.accountName

                        onTextChanged: {
                            fileDelegate.model.accountName = text;
                        }
                    }
                }
            }
        }

        CheckBox {
            id: useCategoriesCheckBox
            text: qsTr("Use categories from CSV")
            checked: false
        }
    }
}
