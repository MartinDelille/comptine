import QtQuick
import QtTest

import ui.common

Item {
    id: root
    width: 400
    height: 80

    Component {
        id: dateLabelComponent
        DateLabel {}
    }

    TestCase {
        name: "DateLabelTests"
        when: windowShown

        function test_rendersMonthAndYear() {
            let label = createTemporaryObject(dateLabelComponent, root, {
                date: new Date(2024, 0, 15)
            });
            verify(!!label, "Component exists");
            compare(label.text, "January 2024");
        }

        function test_monthNameHandlesDecember() {
            let label = createTemporaryObject(dateLabelComponent, root, {
                date: new Date(2025, 11, 1)
            });
            verify(!!label, "Component exists");
            compare(label.monthName(12), "December");
            compare(label.text, "December 2025");
        }
    }
}
