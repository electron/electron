#!/bin/bash

SSHD_PID=$1
CLOUDFLARED_PID=$2
SESSION_TIMEOUT=${3:-10000}
TUNNEL_NAME=$4

cleanup() {
    # Kill processes.
    for pid in "$SLEEP_PID" "$SSHD_PID" "$CLOUDFLARED_PID"; do
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    # Clean up tunnel.
    if [ -n "$TUNNEL_NAME" ]; then
        cd "$GITHUB_ACTION_PATH"
        ./cloudflared tunnel delete "$TUNNEL_NAME" 2>/dev/null || {
            echo "Failed to delete tunnel"
        }
    fi

    echo "Session ended at $(date)"
    exit 0
}

# Trap signals to ensure cleanup.
trap cleanup SIGTERM SIGINT SIGQUIT SIGHUP EXIT

# Wait for timeout or until processes die.
sleep "$SESSION_TIMEOUT" &
SLEEP_PID=$!

# Monitor processes
while kill -0 "$SLEEP_PID" 2>/dev/null; do
    # Check SSH daemon.
    if ! kill -0 "$SSHD_PID" 2>/dev/null; then
        echo "SSH daemon died at $(date)"
        break
    fi

    # Check cloudflared,
    if ! kill -0 "$CLOUDFLARED_PID" 2>/dev/null; then
        echo "Cloudflared died at $(date)"
        break
    fi

    sleep 10
done

cleanup
