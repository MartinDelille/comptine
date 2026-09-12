#!/bin/bash
# Check that all translations are complete

set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"

source_files=()
while IFS= read -r source_file; do
  source_files+=("$project_root/$source_file")
done < <(
  cd "$project_root"
  rg --files -g '*.qml' -g '*.cpp' -g '*.h'
)

# Update translation file with current source strings
# Use -locations none to avoid noisy diffs when line numbers change
lupdate -locations none -no-obsolete "${source_files[@]}" -ts "$project_root/translations/comptine_fr.ts"

# Check for unfinished translations
if grep -q 'type="unfinished"' "$project_root/translations/comptine_fr.ts"; then
  echo "Error: Found unfinished translations:"
  grep -B2 'type="unfinished"' "$project_root/translations/comptine_fr.ts" | grep "<source>"
  exit 1
fi

echo "All translations are complete!"
exit 0
