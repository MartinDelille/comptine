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
    property var fileEntries: []

    okEnabled: {
        if (fileEntries.length === 0)
            return false;
        for (var i = 0; i < fileEntries.length; i++) {
            if (fileEntries[i].accountName.trim() === "")
                return false;
        }
        return true;
    }

    onOpened: {
        var entries = [];
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

            entries.push({
                "url": filePaths[i],
                "fileName": fileName,
                "accountName": suggested,
                "isNewAccount": isNew,
                "existingAccountIndex": existingIndex >= 0 ? existingIndex : 0
            });
        }
        fileEntries = entries;
        useCategoriesCheckBox.checked = false;
    }

    onAccepted: {
        for (var i = 0; i < fileEntries.length; i++) {
            var entry = fileEntries[i];
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
            model: importDialog.fileEntries.length
            spacing: Theme.spacingNormal

            delegate: ColumnLayout {
                id: fileDelegate
                required property int index
                width: parent.width
                spacing: Theme.spacingSmall

                Label {
                    text: importDialog.fileEntries[fileDelegate.index].fileName
                    font.pixelSize: Theme.fontSizeNormal
                    font.bold: true
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: Theme.spacingNormal
                    Layout.fillWidth: true

                    CheckBox {
                        id: newAccountCheck
                        text: qsTr("New account")
                        checked: importDialog.fileEntries[fileDelegate.index].isNewAccount

                        onCheckedChanged: {
                            var entries = importDialog.fileEntries;
                            if (fileDelegate.index < entries.length) {
                                entries[fileDelegate.index].isNewAccount = checked;
                                if (!checked) {
                                    // Switching to existing: use combo selection
                                    var account = AppState.data.accountAt(accountCombo.currentIndex);
                                    entries[fileDelegate.index].accountName = account ? account.name : "";
                                } else {
                                    // Switching to new: use text field value
                                    entries[fileDelegate.index].accountName = newAccountField.text.trim();
                                }
                                importDialog.fileEntries = entries;
                            }
                        }
                    }

                    AccountComboBox {
                        id: accountCombo
                        budgetData: AppState.data
                        Layout.fillWidth: true
                        visible: !newAccountCheck.checked

                        Component.onCompleted: {
                            var entry = importDialog.fileEntries[fileDelegate.index];
                            if (entry)
                                currentIndex = entry.existingAccountIndex;
                        }

                        onCurrentIndexChanged: {
                            if (!newAccountCheck.checked) {
                                var entries = importDialog.fileEntries;
                                if (fileDelegate.index < entries.length) {
                                    var account = AppState.data.accountAt(currentIndex);
                                    entries[fileDelegate.index].accountName = account ? account.name : "";
                                    importDialog.fileEntries = entries;
                                }
                            }
                        }
                    }

                    TextField {
                        id: newAccountField
                        Layout.fillWidth: true
                        visible: newAccountCheck.checked
                        placeholderText: qsTr("Account name")

                        Component.onCompleted: {
                            var entry = importDialog.fileEntries[fileDelegate.index];
                            if (entry && entry.isNewAccount)
                                text = entry.accountName;
                        }

                        onTextChanged: {
                            if (newAccountCheck.checked) {
                                var entries = importDialog.fileEntries;
                                if (fileDelegate.index < entries.length) {
                                    entries[fileDelegate.index].accountName = text.trim();
                                    importDialog.fileEntries = entries;
                                }
                            }
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
