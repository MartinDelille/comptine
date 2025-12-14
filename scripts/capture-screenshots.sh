#!/bin/bash
#
# Capture screenshots of Comptine for documentation
#
# Usage: ./scripts/capture-screenshots.sh <app_path> <data_file> [output_dir]
#
# Arguments:
#   app_path   - Path to Comptine.app (required)
#   data_file  - Path to .comptine file to load (required)
#   output_dir - Directory for screenshots (default: docs/assets/screenshots)
#
# Requirements:
#   - macOS (uses screencapture)
#   - yq (for YAML manipulation)
#   - Screen Recording permission granted to Terminal/shell
#
# Examples:
#   ./scripts/capture-screenshots.sh build/Comptine.app example_long.comptine
#   ./scripts/capture-screenshots.sh /path/to/Comptine.app /path/to/data.comptine ./screenshots

set -e

# Parse arguments
if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <app_path> <data_file> [output_dir]"
  echo ""
  echo "Arguments:"
  echo "  app_path   - Path to Comptine.app (required)"
  echo "  data_file  - Path to .comptine file to load (required)"
  echo "  output_dir - Directory for screenshots (default: docs/assets/screenshots)"
  exit 1
fi

APP_PATH="$1"
DATA_FILE="$2"
OUTPUT_DIR="${3:-docs/assets/screenshots}"

# Configuration
WINDOW_WIDTH=960
WINDOW_HEIGHT=720
WAIT_TIME=3 # seconds to wait for app to render
SCREENSHOT_FILE_NAME="Comptabilité Familiale.comptine"

# Temp directory for modified data files
TEMP_DIR=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

log_info() {
  echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
  echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
  echo -e "${RED}[ERROR]${NC} $1"
}

# Check requirements
check_requirements() {
  if [[ "$(uname)" != "Darwin" ]]; then
    log_error "This script only works on macOS"
    exit 1
  fi

  if [[ ! -d "$APP_PATH" ]]; then
    log_error "App not found at $APP_PATH"
    exit 1
  fi

  if [[ ! -f "$DATA_FILE" ]]; then
    log_error "Data file not found: $DATA_FILE"
    exit 1
  fi

  if ! command -v yq &>/dev/null; then
    log_error "yq is required but not installed. Install with: brew install yq"
    exit 1
  fi
}

# Create output directory
setup_output_dir() {
  mkdir -p "$OUTPUT_DIR"
  log_info "Output directory: $OUTPUT_DIR"
}

# Set light theme for consistent screenshots
set_light_theme() {
  log_info "Setting light appearance for screenshots..."
  defaults write org.martin.delille.Comptine theme "light" 2>/dev/null || true
}

# Create a temporary data file with a specific tab selected
create_temp_data_file() {
  local tab_index="$1"
  local temp_file="$TEMP_DIR/$SCREENSHOT_FILE_NAME"

  # Only copy and modify if not already done for this tab
  # Copy the original file and modify currentTab
  cp "$DATA_FILE" "$temp_file"
  yq -i ".state.currentTab = $tab_index" "$temp_file"

  echo "$temp_file"
}

# Get window ID using Swift (works without special permissions)
get_window_id() {
  swift - <<'EOF'
import Cocoa

let options: CGWindowListOption = [.optionOnScreenOnly, .excludeDesktopElements]
guard let windowList = CGWindowListCopyWindowInfo(options, kCGNullWindowID) as? [[String: Any]] else {
    exit(1)
}

for window in windowList {
    if let ownerName = window[kCGWindowOwnerName as String] as? String,
       ownerName == "Comptine",
       let windowID = window[kCGWindowNumber as String] as? Int {
        print(windowID)
        exit(0)
    }
}
exit(1)
EOF
}

# Wait for window to appear
wait_for_window() {
  local max_attempts=20
  local attempt=0

  log_info "Waiting for Comptine window..."

  while [[ $attempt -lt $max_attempts ]]; do
    if [[ -n "$(get_window_id 2>/dev/null)" ]]; then
      log_info "Window found!"
      return 0
    fi
    sleep 0.5
    ((attempt++))
  done

  log_error "Timeout waiting for window"
  return 1
}

# Launch the app with a specific data file
launch_app() {
  local data_file="$1"
  log_info "Launching Comptine with $data_file..."

  # Get absolute path to data file
  local abs_data_file
  abs_data_file="$(cd "$(dirname "$data_file")" && pwd)/$(basename "$data_file")"

  # Get absolute path to app binary
  local abs_app_path
  abs_app_path="$(cd "$(dirname "$APP_PATH")" && pwd)/$(basename "$APP_PATH")"
  local app_binary="$abs_app_path/Contents/MacOS/Comptine"

  if [[ ! -x "$app_binary" ]]; then
    log_error "App binary not found: $app_binary"
    return 1
  fi

  # Launch the app binary directly (not via 'open') to ensure file argument is passed
  "$app_binary" "$abs_data_file" &
  APP_PID=$!

  # Wait for window to appear
  wait_for_window

  log_info "Waiting ${WAIT_TIME}s for app to fully render..."
  sleep "$WAIT_TIME"
}

# Setup window size and position
setup_window() {
  log_info "Setting window size to ${WINDOW_WIDTH}x${WINDOW_HEIGHT}..."

  osascript <<EOF
tell application "System Events"
    tell process "Comptine"
        set frontmost to true
        delay 0.5
        if (count of windows) > 0 then
            set position of window 1 to {100, 100}
            set size of window 1 to {$WINDOW_WIDTH, $WINDOW_HEIGHT}
        end if
    end tell
end tell
EOF

  sleep 1
}

# Capture a screenshot of the Comptine window
capture_screenshot() {
  local output_file="$1"
  local window_id

  window_id=$(get_window_id 2>/dev/null)
  if [[ -z "$window_id" ]]; then
    log_error "Could not get window ID. Is Comptine running?"
    return 1
  fi

  log_info "Capturing window $window_id to $output_file..."

  # Use screencapture with window ID
  # -l: capture specific window by ID
  # -o: exclude shadow (cleaner for docs)
  # -x: no sound
  screencapture -l"$window_id" -o -x "$output_file"

  if [[ -f "$output_file" ]]; then
    log_info "Screenshot saved: $output_file"
    return 0
  else
    log_error "Failed to capture screenshot"
    return 1
  fi
}

# Quit the app
quit_app() {
  log_info "Quitting Comptine..."
  osascript -e 'tell application "Comptine" to quit' 2>/dev/null || true
  pkill -x Comptine 2>/dev/null || true
  sleep 1
}

# Cleanup on exit
cleanup() {
  quit_app
  if [[ -n "$TEMP_DIR" && -d "$TEMP_DIR" ]]; then
    rm -rf "$TEMP_DIR"
  fi
}

trap cleanup EXIT

# Capture a single screenshot with a specific tab
capture_tab_screenshot() {
  local tab_index="$1"
  local output_file="$2"
  local tab_name

  case "$tab_index" in
  0) tab_name="Operations" ;;
  1) tab_name="Budget" ;;
  *) tab_name="Tab $tab_index" ;;
  esac

  log_info "=== Capturing $tab_name view ==="

  # Kill any existing instance
  pkill -x Comptine 2>/dev/null || true
  sleep 1

  # Create temp file with the correct tab selected
  local temp_file
  temp_file=$(create_temp_data_file "$tab_index")

  # Launch app with this tab
  launch_app "$temp_file"
  setup_window

  # Capture
  capture_screenshot "$output_file"

  # Quit
  quit_app
}

# Main function
main() {
  log_info "=== Comptine Screenshot Capture ==="
  log_info "App: $APP_PATH"
  log_info "Data: $DATA_FILE"

  check_requirements
  setup_output_dir
  set_light_theme

  # Create temp directory for modified data files
  TEMP_DIR=$(mktemp -d)
  log_info "Temp directory: $TEMP_DIR"

  # Capture Operations view (tab 0)
  capture_tab_screenshot 0 "$OUTPUT_DIR/operations.png"

  # Capture Budget view (tab 1)
  capture_tab_screenshot 1 "$OUTPUT_DIR/budget.png"

  log_info "=== Screenshot capture complete ==="
  log_info "Screenshots saved to $OUTPUT_DIR/"
  ls -la "$OUTPUT_DIR/"
}

# Run main function
main
