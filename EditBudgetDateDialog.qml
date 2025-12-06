import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property int operationIndex: -1
    property date originalBudgetDate: new Date()

    title: qsTr("Edit Budget Date")
    standardButtons: Dialog.Ok | Dialog.Cancel
    anchors.centerIn: Overlay.overlay
    modal: true

    contentItem: ColumnLayout {
        spacing: Theme.spacingNormal

        Label {
            text: qsTr("Select the month for budget calculation:")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }

        RowLayout {
            spacing: Theme.spacingNormal

            ComboBox {
                id: budgetMonthCombo
                model: [qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"), qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"), qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")]
                currentIndex: root.originalBudgetDate.getMonth()
            }

            SpinBox {
                id: budgetYearSpin
                from: 2000
                to: 2100
                value: root.originalBudgetDate.getFullYear()
                editable: true
            }
        }
    }

    onAccepted: {
        if (root.operationIndex >= 0) {
            // Create a date on the 1st of the selected month
            let newDate = new Date(budgetYearSpin.value, budgetMonthCombo.currentIndex, 1);
            budgetData.setOperationBudgetDate(root.operationIndex, newDate);
        }
    }

    onAboutToShow: {
        budgetMonthCombo.currentIndex = root.originalBudgetDate.getMonth();
        budgetYearSpin.value = root.originalBudgetDate.getFullYear();
    }
}
