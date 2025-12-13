import QtQuick
import QtQuick.Controls

// Reusable SpinBox for date components (day, year).
// Automatically selects all text on focus for easier replacement.
SpinBox {
    id: root

    editable: true

    // Select all text when focused
    onActiveFocusChanged: {
        if (activeFocus && contentItem) {
            contentItem.selectAll();
        }
    }
}
