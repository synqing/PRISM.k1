#!/bin/bash
# Pre-Claim Hook - Run before agent claims task

set -e

TASK_ID=$1
AGENT_ID=$2

echo "🔍 Pre-claim validation for task $TASK_ID by $AGENT_ID..."

# 1. Sync with Knowledge Fortress
cd ../..
if ! multi-agent/scripts/sync-with-fortress.sh; then
    echo "❌ Failed to sync with Knowledge Fortress"
    exit 1
fi

# 2. Validate current code matches CANON (if validate script exists)
if [ -x ".taskmaster/scripts/validate-canon.sh" ]; then
    cd firmware
    if ! ../.taskmaster/scripts/validate-canon.sh 2>&1 | tail -10; then
        echo "❌ Code does not match CANON. Fix drift before claiming tasks."
        cd ..
        exit 1
    fi
    cd ..
fi

# 3. Check for file locks on files this task will modify
# (This would parse task description for file paths)

echo "✅ Pre-claim validation passed"
