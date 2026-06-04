#!/bin/bash
# Demonstrate parallel multi-agent operation
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$PROJECT_DIR/build/webagent"

if [ ! -f "$BINARY" ]; then
    echo "Binary not found. Run ./scripts/build.sh first."
    exit 1
fi

echo "Starting 3 agents in parallel..."
for i in 1 2 3; do
    mkdir -p "$PROJECT_DIR/tasks_agent$i" "$PROJECT_DIR/results_agent$i"
    "$BINARY" \
        --config "$PROJECT_DIR/config/agent.json" \
        --uid "agent-00$i" \
        &
    echo "  Agent $i started (PID $!)"
done

echo "All agents running. Press Ctrl+C to stop all."
wait
