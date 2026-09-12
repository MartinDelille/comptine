import QtQuick
import QtTest

import ui.operations

Item {
    id: root
    width: 500
    height: 400

    Component {
        id: dateInputComponent
        DateInput {}
    }

    TestCase {
        name: "DateInputTests"
        when: windowShown

        function test_selectedDateIsDisplayed() {
            let input = createTemporaryObject(dateInputComponent, root, {
                selectedDate: new Date(2024, 3, 15)
            });
            verify(!!input, "Component exists");
            let textField = findChild(input, "textField");
            verify(!!textField, "Text field exists");
            compare(textField.text, "15/04/2024");
        }

        function test_focusOpensSelectedMonth() {
            let input = createTemporaryObject(dateInputComponent, root, {
                selectedDate: new Date(2024, 3, 15)
            });
            verify(!!input, "Component exists");
            let textField = findChild(input, "textField");
            let popup = findChild(input, "popup");
            let monthGrid = findChild(input, "monthGrid");
            verify(!!textField, "Text field exists");
            verify(!!popup, "Popup exists");
            verify(!!monthGrid, "Month grid exists");
            textField.forceActiveFocus();
            tryCompare(popup, "visible", true);
            compare(monthGrid.month, 3);
            compare(monthGrid.year, 2024);
        }

        function test_monthNavigationCrossesYearBoundary() {
            let input = createTemporaryObject(dateInputComponent, root, {
                selectedDate: new Date(2024, 0, 15)
            });
            verify(!!input, "Component exists");
            let textField = findChild(input, "textField");
            let previousButton = findChild(input, "previousMonthButton");
            let popup = findChild(input, "popup");
            let monthGrid = findChild(input, "monthGrid");
            verify(!!textField, "Text field exists");
            verify(!!previousButton, "Previous button exists");
            verify(!!popup, "Popup exists");
            verify(!!monthGrid, "Month grid exists");
            textField.forceActiveFocus();
            tryCompare(popup, "visible", true);
            tryCompare(monthGrid, "month", 0);
            previousButton.click();
            tryCompare(monthGrid, "month", 11);
            tryCompare(monthGrid, "year", 2023);
        }

        function test_resetModifierUnlockRestoresReadOnlyState() {
            let input = createTemporaryObject(dateInputComponent, root, {
                readOnly: true,
                unlockOnModifierClick: true
            });
            verify(!!input, "Component exists");
            let textField = findChild(input, "textField");
            verify(!!textField, "Text field exists");
            compare(textField.readOnly, true);
            input._modifierUnlocked = true;
            compare(textField.readOnly, false);
            input.resetModifierUnlock();
            compare(textField.readOnly, true);
        }
    }
}
