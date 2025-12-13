import QtQuick
import QtQuick.Controls

// Reusable text field for entering monetary amounts.
// Handles both decimal separators (. and ,), removes spaces and currency symbols,
// and automatically selects all text on focus.
TextField {
    id: root

    // The numeric value of the amount
    property double value: 0

    // Track if text is being edited by user (to avoid reformatting during typing)
    property bool _userEditing: false

    // Signal emitted when the value changes via user input (on editing finished)
    signal edited(double newValue)

    // Signal emitted on each text change for live updates (e.g., sum recalculation)
    signal liveEdited(double newValue)

    horizontalAlignment: Text.AlignRight

    // Update text from value when not being edited by user
    onValueChanged: {
        if (!_userEditing) {
            text = value.toFixed(2);
        }
    }

    // Initialize text on completion
    Component.onCompleted: {
        text = value.toFixed(2);
    }

    // Track focus for user editing state
    onActiveFocusChanged: {
        if (activeFocus) {
            selectAll();
        } else {
            _userEditing = false;
        }
    }

    // Emit live updates as user types for real-time sum recalculation
    onTextChanged: {
        if (activeFocus) {
            _userEditing = true;
        }
        let parsed = root.parseAmount(text);
        if (!isNaN(parsed)) {
            root.liveEdited(parsed);
        }
    }

    // Parse and normalize the input when editing is finished
    onEditingFinished: {
        let parsed = root.parseAmount(text);
        if (!isNaN(parsed)) {
            root.value = parsed;
            root.edited(parsed);
        }
        // Reset display to normalized format
        text = root.value.toFixed(2);
    }

    // Parse amount string, handling various formats:
    // - Both . and , as decimal separators
    // - Currency symbols (€, $, etc.)
    // - Spaces (thousand separators or formatting)
    // - Negative values with - or parentheses
    function parseAmount(input: string): double {
        if (!input || input.trim() === "")
            return NaN;

        let str = input.trim();

        // Remove currency symbols and common formatting
        str = str.replace(/[€$£¥]/g, "");
        str = str.replace(/\s/g, "");

        // Handle parentheses for negative numbers: (123.45) -> -123.45
        let isNegative = false;
        if (str.startsWith("(") && str.endsWith(")")) {
            isNegative = true;
            str = str.slice(1, -1);
        }

        // Handle leading minus
        if (str.startsWith("-")) {
            isNegative = !isNegative;
            str = str.slice(1);
        }

        // Handle trailing minus (some formats use it)
        if (str.endsWith("-")) {
            isNegative = !isNegative;
            str = str.slice(0, -1);
        }

        // Determine decimal separator:
        // If both . and , exist, the last one is the decimal separator
        let lastDot = str.lastIndexOf(".");
        let lastComma = str.lastIndexOf(",");

        if (lastDot > lastComma) {
            // Dot is decimal separator, remove commas (thousand separators)
            str = str.replace(/,/g, "");
        } else if (lastComma > lastDot) {
            // Comma is decimal separator, remove dots (thousand separators) and replace comma with dot
            str = str.replace(/\./g, "");
            str = str.replace(",", ".");
        }
        // If neither exists or only one exists, parseFloat handles it

        let result = parseFloat(str);
        if (isNaN(result))
            return NaN;

        return isNegative ? -result : result;
    }
}
