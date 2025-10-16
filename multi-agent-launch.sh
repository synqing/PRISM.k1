#!/bin/bash
# Multi-Agent Launcher - Spawns multiple Claude Code instances in parallel
# Each instance works on different tasks simultaneously

PROJECT_ROOT="/Users/spectrasynq/Workspace_Management/Software/PRISM.k1"
cd "$PROJECT_ROOT"

echo "🚀 PRISM K1 Multi-Agent Launcher"
echo "================================="
echo ""

# Check if Claude Code is available
if ! command -v claude &> /dev/null; then
    echo "❌ Claude Code CLI not found"
    echo ""
    echo "Install Claude Code CLI first, then run this script again."
    exit 1
fi

# Check task-master is available
if ! command -v task-master &> /dev/null; then
    echo "❌ task-master CLI not found"
    echo ""
    echo "Install task-master first: npm install -g task-master-ai"
    exit 1
fi

echo "✅ Claude Code CLI found"
echo "✅ task-master CLI found"
echo ""

# Sync with Knowledge Fortress first
echo "📚 Syncing with Knowledge Fortress..."
if ! multi-agent/scripts/sync-with-fortress.sh; then
    echo "❌ Failed to sync with fortress"
    exit 1
fi

echo ""
echo "🤖 Available Agents:"
echo "  1. Network Agent   - WiFi, WebSocket, TLV (Tasks 2, 3, 4)"
echo "  2. Storage Agent   - LittleFS, Patterns (Tasks 5, 6, 7)"
echo "  3. Playback Agent  - LED Driver, Effects (Tasks 8, 9)"
echo "  4. Template Agent  - Pattern Templates (Task 10)"
echo "  5. Integration     - Testing, Optimization (Task 1)"
echo ""

read -p "How many agents to spawn? (1-5): " NUM_AGENTS

if [ -z "$NUM_AGENTS" ] || [ "$NUM_AGENTS" -lt 1 ] || [ "$NUM_AGENTS" -gt 5 ]; then
    echo "Invalid number. Using 1 agent."
    NUM_AGENTS=1
fi

echo ""
echo "🚀 Spawning $NUM_AGENTS Claude Code instances..."
echo ""

# Agent configurations
AGENTS=(
    "integration:1:Component scaffolding"
    "network:2:WiFi lifecycle"
    "network:3:WebSocket server"
    "storage:5:LittleFS storage"
    "playback:8:RMT LED driver"
)

# macOS: Use Terminal.app to spawn new windows
# Linux: Use gnome-terminal or xterm
# Windows: Use Windows Terminal or cmd

detect_terminal() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif command -v gnome-terminal &> /dev/null; then
        echo "gnome"
    elif command -v xterm &> /dev/null; then
        echo "xterm"
    else
        echo "unknown"
    fi
}

TERMINAL_TYPE=$(detect_terminal)

spawn_agent() {
    local AGENT_NUM=$1
    local AGENT_TAG=$2
    local TASK_ID=$3
    local TASK_NAME=$4

    echo "  🤖 Agent $AGENT_NUM ($AGENT_TAG): Task $TASK_ID - $TASK_NAME"

    case "$TERMINAL_TYPE" in
        macos)
            osascript <<EOF
tell application "Terminal"
    activate
    do script "cd '$PROJECT_ROOT' && echo '🤖 Agent $AGENT_NUM: $AGENT_TAG' && echo 'Task $TASK_ID: $TASK_NAME' && echo '' && task-master show --id=$TASK_ID && echo '' && echo '📋 Starting Claude Code...' && echo '' && claude"
end tell
EOF
            ;;
        gnome)
            gnome-terminal -- bash -c "cd '$PROJECT_ROOT'; echo '🤖 Agent $AGENT_NUM: $AGENT_TAG'; echo 'Task $TASK_ID: $TASK_NAME'; task-master show --id=$TASK_ID; claude; exec bash"
            ;;
        xterm)
            xterm -e "cd '$PROJECT_ROOT' && echo '🤖 Agent $AGENT_NUM: $AGENT_TAG' && echo 'Task $TASK_ID: $TASK_NAME' && task-master show --id=$TASK_ID && claude && bash" &
            ;;
        *)
            echo "⚠️  Unknown terminal. Please manually open a new terminal and run:"
            echo "     cd $PROJECT_ROOT"
            echo "     task-master show --id=$TASK_ID"
            echo "     claude"
            ;;
    esac

    sleep 2  # Stagger spawning
}

# Spawn agents
for i in $(seq 1 $NUM_AGENTS); do
    AGENT_INFO="${AGENTS[$((i-1))]}"
    IFS=':' read -r AGENT_TAG TASK_ID TASK_NAME <<< "$AGENT_INFO"
    spawn_agent "$i" "$AGENT_TAG" "$TASK_ID" "$TASK_NAME"
done

echo ""
echo "✅ Spawned $NUM_AGENTS Claude Code instances"
echo ""
echo "📊 Monitor progress:"
echo "  - Watch task status: task-master list"
echo "  - Check agent logs: multi-agent/logs/"
echo "  - Dashboard (if available): multi-agent/scripts/dashboard.sh"
echo ""
echo "⚠️  Each Claude Code instance is independent."
echo "    They will coordinate through task-master and CANON validation."
echo ""
echo "🛑 To stop all agents: Close the terminal windows manually"
echo ""
