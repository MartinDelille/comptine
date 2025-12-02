import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: preferencesDialog
    title: qsTr("Preferences")
    standardButtons: Dialog.Ok | Dialog.Cancel
    modal: true

    property string originalLanguage: ""

    onOpened: {
        // Save original language to restore on cancel
        originalLanguage = appSettings.language;

        // Set initial combo box value
        if (appSettings.language === "") {
            languageComboBox.currentIndex = 0;
        } else if (appSettings.language === "en") {
            languageComboBox.currentIndex = 1;
        } else if (appSettings.language === "fr") {
            languageComboBox.currentIndex = 2;
        }
    }

    onRejected: {
        // Restore original language on cancel
        if (appSettings.language !== originalLanguage) {
            appSettings.language = originalLanguage;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        GridLayout {
            columns: 2
            columnSpacing: 10
            rowSpacing: 10

            Label {
                text: qsTr("Language:")
            }

            ComboBox {
                id: languageComboBox
                model: [qsTr("System Default"), "English", "Français"]

                onActivated: {
                    var newLanguage = "";
                    if (currentIndex === 1) {
                        newLanguage = "en";
                    } else if (currentIndex === 2) {
                        newLanguage = "fr";
                    }
                    appSettings.language = newLanguage;
                }
            }
        }
    }
}
