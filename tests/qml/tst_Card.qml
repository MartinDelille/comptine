import QtQuick
import QtTest

import ui.common

Item {
    id: root
    width: 400
    height: 80

    Component {
        id: cardComponent
        Card {}
    }

    TestCase {
        name: "CardTests"
        when: windowShown

        function test_defaultThemeStyle() {
            let card = createTemporaryObject(cardComponent, root);
            verify(!!card, "Component exists");
            compare(card.color, Theme.surface);
            compare(card.border.color, Theme.border);
            compare(card.border.width, Theme.cardBorderWidth);
            compare(card.radius, Theme.cardRadius);
        }

        function test_styleCanBeOverridden() {
            let card = createTemporaryObject(cardComponent, root, {
                color: "red",
                radius: 3
            });
            verify(!!card, "Component exists");
            compare(card.color, "#ff0000");
            compare(card.radius, 3);
        }
    }
}
