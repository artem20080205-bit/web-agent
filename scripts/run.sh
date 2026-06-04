#!/bin/bash
# Run the web-agent (from build/ directory)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/build/webagent"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Run ./scripts/build.sh first."
    exit 1
fi

CONFIG="${1:-$PROJECT_DIR/config/agent.json}"
echo "Starting webagent with config: $CONFIG"
exec "$BINARY" --config "$CONFIG"
