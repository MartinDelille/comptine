import QtQuick
import QtTest

import ui.common

Item {
    id: root
    width: 400
    height: 80

    Component {
        id: amountFieldComponent
        AmountField {}
    }

    SignalSpy {
        id: editedSpy
        signalName: "edited"
    }

    TestCase {
        name: "AmountFieldTests"
        when: windowShown

        function test_initialValueIsFormatted() {
            let field = createTemporaryObject(amountFieldComponent, root, {
                value: 1234.5
            });
            verify(!!field, "Component exists");
            compare(field.text, "1234.50");
        }

        function test_parseAmountSupportsCommonFormats() {
            let field = createTemporaryObject(amountFieldComponent, root);
            verify(!!field, "Component exists");
            verify(Math.abs(field.parseAmount("1 234,56 €") - 1234.56) < 0.001);
            verify(Math.abs(field.parseAmount("(12.50)") + 12.5) < 0.001);
            verify(Math.abs(field.parseAmount("12.50-") + 12.5) < 0.001);
            verify(isNaN(field.parseAmount("not an amount")));
        }

        function test_editingEmitsParsedValue() {
            let field = createTemporaryObject(amountFieldComponent, root);
            verify(!!field, "Component exists");
            editedSpy.target = field;
            editedSpy.clear();
            field.forceActiveFocus();
            field.text = "12,34";
            tryCompare(editedSpy, "count", 1);
            compare(editedSpy.signalArguments[0][0], 12.34);
        }
    }
}
