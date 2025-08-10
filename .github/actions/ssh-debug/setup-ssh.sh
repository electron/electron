#!/bin/bash -e

get_authorized_keys() {
    if [ -z "$AUTHORIZED_USERS" ] || ! echo "$AUTHORIZED_USERS" | grep -q "\b$GITHUB_ACTOR\b"; then
        return 1
    fi

    api_response=$(curl -s "https://api.github.com/users/$GITHUB_ACTOR/keys")

    if echo "$api_response" | jq -e 'type == "object" and has("message")' >/dev/null; then
        error_msg=$(echo "$api_response" | jq -r '.message')
        echo "Error: $error_msg"
        return 1
    else
        echo "$api_response" | jq -r '.[].key'
    fi
}

authorized_keys=$(get_authorized_keys "$GITHUB_ACTOR")

if [ -n "$authorized_keys" ]; then
    echo "Configured SSH key(s) for user: $GITHUB_ACTOR"
else
    echo "Error: User '$GITHUB_ACTOR' is not authorized to access this debug session."
    echo "Authorized users: $AUTHORIZED_USERS"
    exit 1
fi

if [ "$TUNNEL" != "true" ]; then
    echo "SSH tunneling is disabled. Set enable-tunnel: true to enable remote access."
    echo "Local SSH server would be available on localhost:2222 if this were a local environment."
    exit 0
fi

echo "SSH tunneling enabled. Setting up remote access..."

EXTERNAL_DEPS="curl jq ssh-keygen"

for dep in $EXTERNAL_DEPS; do
    if ! command -v "$dep" > /dev/null 2>&1; then
       echo "Command $dep not installed on the system!" >&2
       exit 1
    fi
done

cd "$GITHUB_ACTION_PATH"

bashrc_path=$(pwd)/bashrc

# Source `bashrc` to auto start tmux on SSH login.
if ! grep -q "$bashrc_path" ~/.bash_profile; then
    echo >> ~/.bash_profile # On macOS runner there's no newline at the end of the file
    echo "source \"$bashrc_path\"" >> ~/.bash_profile
fi

OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

if [ "$ARCH" = "x86_64" ]; then
    ARCH="amd64"
elif [ "$ARCH" = "aarch64" ]; then
    ARCH="arm64"
fi

# Install tmux on macOS runners if not present.
if [ "$OS" = "darwin" ] && ! command -v tmux > /dev/null 2>&1; then
    echo "Installing tmux..."
    brew install tmux
fi

if [ "$OS" = "darwin" ]; then
    cloudflared_url="https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-${OS}-${ARCH}.tgz"
    echo "Downloading \`cloudflared\` from <$cloudflared_url>..."
    curl --location --silent --output cloudflared.tgz "$cloudflared_url"
    tar xf cloudflared.tgz
    rm cloudflared.tgz
else
    cloudflared_url="https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-${OS}-${ARCH}"
    echo "Downloading \`cloudflared\` from <$cloudflared_url>..."
    curl --location --silent --output cloudflared "$cloudflared_url"
fi

chmod +x cloudflared

echo "Setting up SSH key for authorized user: $GITHUB_ACTOR"
echo "$authorized_keys" > authorized_keys

echo 'Creating SSH server key...'
ssh-keygen -q -f ssh_host_rsa_key -N ''

echo 'Creating SSH server config...'
sed "s,\$PWD,$PWD,;s,\$USER,$USER," sshd_config.template > sshd_config

echo 'Starting SSH server...'
/usr/sbin/sshd -f sshd_config -D &
sshd_pid=$!

echo 'Starting tmux session...'
(cd "$GITHUB_WORKSPACE" && tmux new-session -d -s debug)

#if no cloudflare tunnel token is provided, exit
if [ -z "$CLOUDFLARE_TUNNEL_TOKEN" ]; then
    echo "Error: required CLOUDFLARE_TUNNEL_TOKEN not found"
    exit 1
fi

echo 'Starting Cloudflare tunnel...'

./cloudflared tunnel --no-autoupdate run --token "$CLOUDFLARE_TUNNEL_TOKEN" 2>&1 | tee cloudflared.log | sed -u 's/^/cloudflared: /' &
cloudflared_pid=$!

url="$TUNNEL_HOSTNAME"

public_key=$(cut -d' ' -f1,2 < ssh_host_rsa_key.pub)

(
    echo '    '
    echo '    '
    echo '🔗 SSH Debug Session Ready!'
    echo '━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━'
    echo '    '
    echo '📋 Copy and run this command to connect:'
    echo '    '
    if [ -n "$TUNNEL_HOSTNAME" ]; then
        echo "ssh-keygen -R action-ssh-debug && echo 'action-ssh-debug $public_key' >> ~/.ssh/known_hosts && ssh -o ProxyCommand='cloudflared access tcp --hostname $url' runner@action-ssh-debug"
    else
        echo "ssh-keygen -R action-ssh-debug && echo 'action-ssh-debug $public_key' >> ~/.ssh/known_hosts && ssh -o ProxyCommand='cloudflared access tcp --hostname $url' runner@action-ssh-debug"
    fi
    echo '    '
    echo "⏰ Session expires automatically in $TIMEOUT minutes"
    echo '━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━'
    echo '    '
    echo '    '
) | cat

echo 'Starting SSH session in background...'
./ssh-session.sh "$sshd_pid" "$cloudflared_pid" $TIMEOUT &

echo 'SSH session is running in background. GitHub Action will continue.'
echo 'Session will auto-cleanup after timeout or when processes end.'
