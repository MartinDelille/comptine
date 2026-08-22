pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Comptine
import commonui
import operations

BaseDialog {
    title: qsTr("Import CSV Files")
    acceptButtonText: qsTr("Import All")
    width: 600

    property var fileUrls: []

    // Model built from fileUrls with per-file account info
    ListModel {
        id: fileEntries
    }

    okEnabled: fileEntries.count > 0

    onOpened: {
        fileEntries.clear();
        for (var i = 0; i < fileUrls.length; i++) {
            var fileUrl = fileUrls[i];
            var suggestedAccountName = BudgetData.suggestedAccountForUrl(fileUrl);
            var account = BudgetData.accountByName(suggestedAccountName);
            var isNew = account === null;

            fileEntries.append({
                url: fileUrl,
                accountName: suggestedAccountName,
                isNewAccount: isNew,
                existingAccountIndex: account ? BudgetData.accountIndex(account) : -1
            });
        }
        useCategoriesCheckBox.checked = false;
    }

    onAccepted: {
        for (var i = 0; i < fileEntries.count; i++) {
            var entry = fileEntries.get(i);
            var account = BudgetData.at(entry.existingAccountIndex);
            if (!entry.isNewAccount && account === null) {
                continue;
            }
            var accountName = entry.isNewAccount ? entry.accountName.trim() : account.name;
            FileController.importFromCsv(entry.url, accountName, useCategoriesCheckBox.checked);
            if (account) {
                account.addImportSourcePrefix(entry.accountName);
            }
        }
        BudgetData.currentTabIndex = 0;
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
                width: parent?.width || 0
                spacing: Theme.spacingSmall

                RowLayout {
                    Label {
                        property string home: StandardPaths.writableLocation(StandardPaths.HomeLocation)
                        text: fileDelegate.model.url.toString().replace(home, "~").replace(/^file:\/\//, "")
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
                            if (!checked) {
                                accountCombo.currentIndex = BudgetData.accountCount > 0 ? 0 : -1;
                            }
                        }
                    }

                    AccountComboBox {
                        id: accountCombo
                        Layout.fillWidth: true
                        visible: !newAccountCheck.checked
                        currentIndex: fileDelegate.model.existingAccountIndex
                        onCurrentIndexChanged: fileDelegate.model.existingAccountIndex = currentIndex
                    }

                    TextField {
                        id: newAccountField
                        Layout.fillWidth: true
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
