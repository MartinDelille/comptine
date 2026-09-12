import QtQuick
import QtTest

import ui.common

Item {
    id: root
    width: 400
    height: 80

    TestCase {
        name: "ThemeTests"
        when: windowShown

        function test_formatAmount() {
            compare(Theme.formatAmountWithoutCurrency(1234.5), "1234,50");
            compare(Theme.formatAmount(-12.3), "-12,30 €");
        }

        function test_amountColor() {
            compare(Theme.amountColor(1), Theme.positive);
            compare(Theme.amountColor(-1), Theme.negative);
        }

        function test_balanceColor() {
            compare(Theme.balanceColor(-1), Theme.negative);
            compare(Theme.balanceColor(0), Theme.textPrimary);
        }
    }
}
