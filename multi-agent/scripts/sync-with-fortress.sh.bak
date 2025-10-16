#!/bin/bash
# Sync Multi-Agent with Knowledge Fortress

set -e

PROJECT_ROOT="/Users/spectrasynq/Workspace_Management/Software/PRISM.k1"
cd "$PROJECT_ROOT"

# Check CANON freshness
if [ ! -f ".taskmaster/CANON.md" ]; then
    echo "❌ CANON.md not found. Knowledge Fortress incomplete."
    exit 1
fi

CANON_HASH=$(shasum -a 256 .taskmaster/CANON.md | cut -d' ' -f1)
CACHED_HASH=$(cat multi-agent/.canon-hash 2>/dev/null || echo "none")

if [ "$CANON_HASH" != "$CACHED_HASH" ]; then
    echo "📜 CANON updated (hash: ${CANON_HASH:0:8}...)"

    # Invalidate any active claims (agents must re-read CANON)
    if [ -d "multi-agent/.locks" ]; then
        ACTIVE_CLAIMS=$(find multi-agent/.locks -name "*.claim" 2>/dev/null | wc -l)
        if [ $ACTIVE_CLAIMS -gt 0 ]; then
            echo "⚠️  $ACTIVE_CLAIMS active tasks - agents will be notified"
        fi
    fi

    # Broadcast update to all agents
    if [ -x "multi-agent/scripts/message-bus.sh" ]; then
        multi-agent/scripts/message-bus.sh post \
            "system" "all" "canon-updated" \
            "CANON.md changed. Re-read specifications before claiming new tasks."
    fi

    # Update cached hash
    echo "$CANON_HASH" > multi-agent/.canon-hash

    echo "✅ Fortress sync complete"
    exit 0
else
    echo "✓ CANON up to date"
    exit 0
fi
