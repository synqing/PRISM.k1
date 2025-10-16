#!/bin/bash
# Conflict Guard - Prevent file-level conflicts

check_file_locks() {
    local AGENT_ID=$1
    shift
    local FILES=("$@")
    
    for FILE in "${FILES[@]}"; do
        local LOCK_FILE="multi-agent/.file-locks/$(echo $FILE | tr '/' '_').lock"
        
        if [ -f "$LOCK_FILE" ]; then
            local OWNER=$(cat "$LOCK_FILE" | cut -d: -f1)
            if [ "$OWNER" != "$AGENT_ID" ]; then
                echo "CONFLICT: File $FILE locked by $OWNER"
                return 1
            fi
        fi
    done
    
    # Claim files
    mkdir -p multi-agent/.file-locks
    for FILE in "${FILES[@]}"; do
        local LOCK_FILE="multi-agent/.file-locks/$(echo $FILE | tr '/' '_').lock"
        echo "$AGENT_ID:$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "$LOCK_FILE"
    done
    
    echo "✅ Files locked successfully"
    return 0
}

release_file_locks() {
    local AGENT_ID=$1
    
    find multi-agent/.file-locks -type f 2>/dev/null | while read LOCK; do
        if grep -q "^${AGENT_ID}:" "$LOCK" 2>/dev/null; then
            rm "$LOCK"
        fi
    done
    
    echo "✅ File locks released"
}

# CLI
case "$1" in
    check) shift; check_file_locks "$@" ;;
    release) release_file_locks "$2" ;;
    *) echo "Usage: conflict-guard.sh check|release <agent-id> [files...]" ;;
esac
