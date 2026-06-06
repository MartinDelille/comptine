#!/bin/bash
# Check that all translations are complete

# Update translation file with current source strings
# Use -locations none to avoid noisy diffs when line numbers change
lupdate -locations none -no-obsolete **/*.qml **/*.cpp **/*.h -ts translations/comptine_fr.ts 2>/dev/null

# Check for unfinished translations
if grep -q 'type="unfinished"' translations/comptine_fr.ts; then
  echo "Error: Found unfinished translations:"
  grep -B2 'type="unfinished"' translations/comptine_fr.ts | grep "<source>"
  exit 1
fi

echo "All translations are complete!"
exit 0
