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

    // Handle undo/redo: always forward to app's undo stack and clear focus
    // The app's undo stack handles all data changes, TextField's internal undo is not used
    Keys.onPressed: event => {
        if (event.modifiers & Qt.ControlModifier) {
            let isUndo = event.key === Qt.Key_Z && !(event.modifiers & Qt.ShiftModifier);
            let isRedo = (event.key === Qt.Key_Z && (event.modifiers & Qt.ShiftModifier)) || event.key === Qt.Key_Y;

            if (isUndo || isRedo) {
                focus = false;
                if (isUndo)
                    AppState.undoStack.undo();
                else
                    AppState.undoStack.redo();
                event.accepted = true;
            }
        }
    }

    // Update text from value when not being edited by user,
    // or when the external value differs significantly from what's displayed
    onValueChanged: {
        if (!_userEditing) {
            text = value.toFixed(2);
        } else {
            // Even during editing, update if external value differs from displayed
            // This handles undo/redo while field has focus
            let displayedValue = parseAmount(text);
            if (isNaN(displayedValue) || Math.abs(displayedValue - value) > 0.001) {
                text = value.toFixed(2);
            }
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
    // Only emit when user is actively editing (has focus), not during initialization
    onTextChanged: {
        if (activeFocus) {
            _userEditing = true;
            let parsed = root.parseAmount(text);
            if (!isNaN(parsed)) {
                root.liveEdited(parsed);
            }
        }
    }

    // Parse and normalize the input when editing is finished
    onEditingFinished: {
        let parsed = root.parseAmount(text);
        if (!isNaN(parsed)) {
            // Don't set root.value here - it would break the binding!
            // Instead, emit the edited signal and let the model update the value,
            // which will flow back through the binding.
            root.edited(parsed);
        }
        // Reset display to normalized format based on current value
        // (the value may have been updated by the model in response to edited signal)
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
