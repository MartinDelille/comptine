import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import commonui

BaseDialog {
    id: root
    title: qsTr("Preferences")

    required property var settings

    property string originalLanguage: ""
    property string originalTheme: ""
    property bool originalCheckForUpdates: true

    onOpened: {
        // Save original values to restore on cancel
        originalLanguage = settings.language;
        originalTheme = settings.theme;
        originalCheckForUpdates = settings.checkForUpdates;

        // Set initial language combo box value
        if (settings.language === "") {
            languageComboBox.currentIndex = 0;
        } else if (settings.language === "en") {
            languageComboBox.currentIndex = 1;
        } else if (settings.language === "fr") {
            languageComboBox.currentIndex = 2;
        }

        // Set initial theme combo box value
        if (settings.theme === "") {
            themeComboBox.currentIndex = 0;
        } else if (settings.theme === "light") {
            themeComboBox.currentIndex = 1;
        } else if (settings.theme === "dark") {
            themeComboBox.currentIndex = 2;
        }

        // Set initial update checkbox value
        updateCheckBox.checked = settings.checkForUpdates;
    }

    onRejected: {
        // Restore original values on cancel
        if (settings.language !== originalLanguage) {
            settings.language = originalLanguage;
        }
        if (settings.theme !== originalTheme) {
            settings.theme = originalTheme;
        }
        if (settings.checkForUpdates !== originalCheckForUpdates) {
            settings.checkForUpdates = originalCheckForUpdates;
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
                    root.settings.language = newLanguage;
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
                    root.settings.theme = newTheme;
                }
            }

            Label {
                text: qsTr("Updates:")
            }

            CheckBox {
                id: updateCheckBox
                text: qsTr("Check for updates on startup")
                onToggled: {
                    root.settings.checkForUpdates = checked;
                }
            }
        }
    }
}
