pragma Singleton
import QtQuick
import Comptine

QtObject {
    id: theme

    // Detect system dark mode
    readonly property bool systemIsDark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // Determine if we should use dark mode
    readonly property bool isDark: {
        if (AppState.settings.theme === "dark")
            return true;
        if (AppState.settings.theme === "light")
            return false;
        return systemIsDark; // System default
    }

    // Background colors
    readonly property color background: isDark ? "#1a1a1a" : "#ffffff"
    readonly property color backgroundAlt: isDark ? "#242424" : "#f9f9f9"
    readonly property color backgroundHover: isDark ? "#2a2a2a" : "#f5f5f5"
    readonly property color backgroundSelected: isDark ? "#1e3a5f" : "#e3f2fd"
    readonly property color surface: isDark ? "#2d2d2d" : "#fafafa"
    readonly property color surfaceElevated: isDark ? "#363636" : "#ffffff"

    // Text colors
    readonly property color textPrimary: isDark ? "#e0e0e0" : "#333333"
    readonly property color textSecondary: isDark ? "#a0a0a0" : "#666666"
    readonly property color textMuted: isDark ? "#707070" : "#999999"

    // Border colors
    readonly property color border: isDark ? "#404040" : "#dddddd"
    readonly property color borderLight: isDark ? "#353535" : "#e0e0e0"

    // Accent colors
    readonly property color positive: isDark ? "#4caf50" : "#388e3c"
    readonly property color negative: isDark ? "#ef5350" : "#d32f2f"
    readonly property color warning: isDark ? "#ffb74d" : "#f57c00"
    readonly property color accent: isDark ? "#64b5f6" : "#1976d2"

    // Progress bar colors
    readonly property color progressBackground: isDark ? "#404040" : "#e0e0e0"
    readonly property color progressFill: positive

    // Card styling
    readonly property int cardRadius: 8
    readonly property int cardBorderWidth: 1

    // Shadows (more visible in light mode)
    readonly property color shadow: isDark ? "#00000000" : "#20000000"

    // Font sizes
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeNormal: 14
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeXLarge: 20

    // Spacing
    readonly property int spacingSmall: 6
    readonly property int spacingNormal: 10
    readonly property int spacingLarge: 15
    readonly property int spacingXLarge: 20

    // Helper function to format currency
    function formatAmount(amount: real): string {
        return amount.toFixed(2).replace('.', ',') + " €";
    }

    // Get amount color based on value
    function amountColor(amount: real): color {
        return amount < 0 ? negative : positive;
    }

    // Get balance color based on value
    function balanceColor(balance: real): color {
        return balance < 0 ? negative : textPrimary;
    }
}
