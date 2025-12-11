import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: preferencesDialog
    title: qsTr("Preferences")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true

    property string originalLanguage: ""
    property string originalTheme: ""
    property bool originalCheckForUpdates: true

    onOpened: {
        // Save original values to restore on cancel
        originalLanguage = AppState.settings.language;
        originalTheme = AppState.settings.theme;
        originalCheckForUpdates = AppState.settings.checkForUpdates;

        // Set initial language combo box value
        if (AppState.settings.language === "") {
            languageComboBox.currentIndex = 0;
        } else if (AppState.settings.language === "en") {
            languageComboBox.currentIndex = 1;
        } else if (AppState.settings.language === "fr") {
            languageComboBox.currentIndex = 2;
        }

        // Set initial theme combo box value
        if (AppState.settings.theme === "") {
            themeComboBox.currentIndex = 0;
        } else if (AppState.settings.theme === "light") {
            themeComboBox.currentIndex = 1;
        } else if (AppState.settings.theme === "dark") {
            themeComboBox.currentIndex = 2;
        }

        // Set initial update checkbox value
        updateCheckBox.checked = AppState.settings.checkForUpdates;
    }

    onRejected: {
        // Restore original values on cancel
        if (AppState.settings.language !== originalLanguage) {
            AppState.settings.language = originalLanguage;
        }
        if (AppState.settings.theme !== originalTheme) {
            AppState.settings.theme = originalTheme;
        }
        if (AppState.settings.checkForUpdates !== originalCheckForUpdates) {
            AppState.settings.checkForUpdates = originalCheckForUpdates;
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
                    AppState.settings.language = newLanguage;
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
                    AppState.settings.theme = newTheme;
                }
            }

            Label {
                text: qsTr("Updates:")
            }

            CheckBox {
                id: updateCheckBox
                text: qsTr("Check for updates on startup")
                onToggled: {
                    AppState.settings.checkForUpdates = checked;
                }
            }
        }
    }
}
