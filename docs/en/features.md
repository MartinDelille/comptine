---
layout: page
lang: en
title: Features
permalink: /en/features/
---

## File management

- **New**: Create a new empty budget (File > New or Cmd+N)
- **Open**: Load a budget file in YAML format (File > Open or Cmd+O)
- **Save**: Save your budget (File > Save or Cmd+S)
- **Save As**: Save to a new file (File > Save As or Cmd+Shift+S)
- **Recent files**: Quick access to recently opened files

## Account management

- **Multiple accounts**: Manage multiple accounts in a single budget file
- **Account selector**: Easily switch accounts via the dropdown menu
- **Rename**: Rename your accounts as needed

## Operations

- **Operations list**: View all operations sorted by date
- **Details**: View detailed information for each operation
- **Multiple selection**: Single click, Ctrl+click, Shift+click, Cmd+A
- **Keyboard navigation**: Up/Down arrows to navigate
- **Running balance**: Automatic balance calculation for each operation
- **Copy**: Copy selected operations as CSV (Cmd+C)
- **Edit**: Edit details via the edit button or Ctrl+E
  - Amount, date, budget date
  - Split across multiple categories
  - All changes are undoable

## CSV Import

- **Easy import**: Import your bank statements (File > Import CSV)
- **Auto-detection**: Detects delimiter and encoding
- **Column mapping**: Automatic column recognition
- **French banks**: Compatible with French bank formats
- **Duplicate detection**: Avoids duplicate imports
- **Optional categories**: Import categories from CSV

## Budget view

- **Monthly budget**: View your budget month by month
- **Category tracking**: Track spending against defined limits
- **Time navigation**: Navigate between months to view history
- **Accumulated leftover**: Budget display shows carried-over amounts

## Budget leftover

Manage unspent budget at the end of each month:

- **Leftover dialog**: Access via "Leftover..." button in Budget View
- **Per-category decisions**: For each category with unspent budget, choose to:
  - **Save**: Transfer leftover to personal savings
  - **Report**: Carry forward to increase next month's budget
- **Accumulated leftovers**: Reported amounts add to future months' limits
- **Monthly summary**: View totals for savings and transfers
- **Undoable**: All leftover decisions support undo/redo

## Undo / Redo

- **Undo**: Undo the last action (Edit > Undo or Cmd+Z)
- **Redo**: Redo the undone action (Edit > Redo or Cmd+Shift+Z)

### Undoable actions

- Account rename
- CSV import
- Category modification
- Category assignment
- Operation split
- Amount, date, budget date modification
- Leftover decision

## User interface

- **Tabs**: Switch between Operations and Budget
- **Themes**: Light, Dark, or System
- **Languages**: French and English
- **Preferences**: Configure the application (Cmd+,)

## Updates

- **Automatic check**: Checks for new versions on startup
- **Manual check**: Help > Check for Updates
- **Release notes**: View what's new
- **Download link**: Direct access to GitHub releases

## Data format

- **YAML storage**: Human-readable and manually editable format
- **Precision**: Amounts stored with 2 decimal places
