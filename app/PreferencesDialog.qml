import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import ui.common
import services

BaseDialog {
    id: root
    title: qsTr("Preferences")

    property string originalLanguage: ""
    property string originalTheme: ""
    property bool originalCheckForUpdates: true

    onOpened: {
        // Save original values to restore on cancel
        originalLanguage = AppSettings.language;
        originalTheme = AppSettings.theme;
        originalCheckForUpdates = AppSettings.checkForUpdates;

        // Set initial language combo box value
        if (AppSettings.language === "") {
            languageComboBox.currentIndex = 0;
        } else if (AppSettings.language === "en") {
            languageComboBox.currentIndex = 1;
        } else if (AppSettings.language === "fr") {
            languageComboBox.currentIndex = 2;
        }

        // Set initial theme combo box value
        if (AppSettings.theme === "") {
            themeComboBox.currentIndex = 0;
        } else if (AppSettings.theme === "light") {
            themeComboBox.currentIndex = 1;
        } else if (AppSettings.theme === "dark") {
            themeComboBox.currentIndex = 2;
        }

        // Set initial update checkbox value
        updateCheckBox.checked = AppSettings.checkForUpdates;
    }

    onRejected: {
        // Restore original values on cancel
        if (AppSettings.language !== originalLanguage) {
            AppSettings.language = originalLanguage;
        }
        if (AppSettings.theme !== originalTheme) {
            AppSettings.theme = originalTheme;
        }
        if (AppSettings.checkForUpdates !== originalCheckForUpdates) {
            AppSettings.checkForUpdates = originalCheckForUpdates;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        GridLayout {
            columns: 2
            columnSpacing: 20
            rowSpacing: 15

            Label {
                text: qsTr("Language:")
            }

            ComboBox {
                id: languageComboBox
                Layout.preferredWidth: 200
                model: [qsTr("System Default"), "English", "Français"]

                onActivated: {
                    var newLanguage = "";
                    if (currentIndex === 1) {
                        newLanguage = "en";
                    } else if (currentIndex === 2) {
                        newLanguage = "fr";
                    }
                    AppSettings.language = newLanguage;
                }
            }

            Label {
                text: qsTr("Theme:")
            }

            ComboBox {
                id: themeComboBox
                Layout.preferredWidth: 200
                model: [qsTr("System Default"), qsTr("Light"), qsTr("Dark")]

                onActivated: {
                    var newTheme = "";
                    if (currentIndex === 1) {
                        newTheme = "light";
                    } else if (currentIndex === 2) {
                        newTheme = "dark";
                    }
                    AppSettings.theme = newTheme;
                }
            }

            Label {
                text: qsTr("Updates:")
            }

            CheckBox {
                id: updateCheckBox
                text: qsTr("Check for updates on startup")
                onToggled: {
                    AppSettings.checkForUpdates = checked;
                }
            }
        }
    }
}
