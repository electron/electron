# If we're in an interactive SSH session and we're not already in tmux and there's no explicit SSH command, auto attach tmux
if [ -n "$SSH_TTY" ] && [ -z "$TMUX" ] && [ -z "$SSH_ORIGINAL_COMMAND" ]; then
    exec tmux attach || exec tmux
fi
