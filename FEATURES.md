# Comptine - Features

## File Management

- **New**: Create a new empty budget (File > New or Cmd+N)
- **Open**: Load budget data from YAML file (File > Open or Cmd+O)
- **Save**: Save budget data to YAML file (File > Save or Cmd+S)
- **Save As**: Save budget data to a new YAML file (File > Save As or Cmd+Shift+S)
- **Recent Files**: Automatically remembers and reopens last used file

## Account Management

- **Multiple Accounts**: Support for multiple accounts in a single budget file
- **Account Selector**: ComboBox to switch between accounts (disabled when no accounts exist)
- **Account Creation**: Create new accounts when importing CSV data
- **Rename Account**: Rename the current account via the Rename button

## Operations

- **Operation List**: View all operations for the current account sorted by date (most recent first)
- **Operation Details**: View detailed information for selected operation(s)
- **Selection**: Single click, Ctrl+click (toggle), Shift+click (range), Cmd+A (select all)
- **Keyboard Navigation**: Up/Down arrows to navigate, with Shift for extending selection
- **Balance Calculation**: Running balance calculated and displayed for each operation
- **Copy to Clipboard**: Copy selected operations as CSV (Cmd+C)
- **Edit Operation**: Edit operation details via the edit button (✏️) or menu (Ctrl+E)
  - Edit amount (with undo support)
  - Edit date (day/month/year spinboxes)
  - Edit budget date (for deferred budget calculations)
  - Split operation across multiple categories with specific amounts
  - All changes are undoable

## Import

- **CSV Import**: Import operations from CSV files (File > Import CSV)
- **Auto-detect Format**: Automatically detects delimiter (comma or semicolon) and encoding
- **Column Detection**: Automatically maps CSV columns (date, description, amount, category)
- **French Bank Support**: Handles French number format and common French bank CSV exports
- **Duplicate Detection**: Skips operations that already exist (same date, amount, and description)
- **Account Selection**: Choose existing account or create new one when importing
- **Use Categories Option**: Optionally import categories from CSV (new categories auto-created with budget limit 0)
- **Post-Import Selection**: All imported operations are automatically selected after import

## Budget View

- **Monthly Budget**: View budget summary by month
- **Category Tracking**: Track spending by category with budget limits
- **Month Navigation**: Navigate between months to view historical data

## Undo/Redo

- **Undo**: Undo the last action (Edit > Undo or Cmd+Z)
- **Redo**: Redo the last undone action (Edit > Redo or Cmd+Shift+Z)

### Undoable Actions

The following actions modify user data and support undo/redo:

- **Account rename**: Renaming an account via the Rename button
- **CSV import**: Importing operations (and any new categories created during import)
- **Category edits**: Renaming a category or changing its budget limit
- **Operation category**: Setting or clearing an operation's category
- **Operation split**: Splitting an operation across multiple categories
- **Operation amount**: Editing an operation's amount
- **Operation date**: Editing an operation's date
- **Operation budget date**: Changing when an operation counts in the budget

### Non-Undoable Actions (by design)

- **File operations**: Save, Save As, New, Open - these persist immediately to disk
- **Navigation state**: Tab selection, month selection, operation selection - UI state, not data
- **Account deletion**: Destructive action not yet implemented with undo support
- **Preferences**: Theme and language changes take effect immediately

## User Interface

- **Tabs**: Switch between Operations view and Budget view
- **Theme Support**: Light, Dark, and System theme options
- **Localization**: Support for English and French languages
- **Preferences**: Configure language and theme (Cmd+,)

## Updates

- **Auto-Update Check**: Automatically checks for new releases on startup (once per day)
- **Manual Check**: Check for updates via Help > Check for Updates
- **Release Notes**: View release notes for new versions
- **Download Link**: Opens GitHub releases page for easy download
- **Preferences**: Enable or disable automatic update checks in Preferences

## Data Format

- **YAML Storage**: Human-readable YAML format for budget data
- **Precision**: Amounts stored with 2 decimal places for accuracy
