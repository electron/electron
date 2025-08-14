#!/bin/bash -e

get_authorized_keys() {
    [[ -z "$AUTHORIZED_USERS" || -z "$GITHUB_ACTOR" ]] && return 1
    [[ ",$AUTHORIZED_USERS," == *",$GITHUB_ACTOR,"* ]] || return 1

    api_response=$(curl -s -H "Authorization: token ${GITHUB_TOKEN}" "https://api.github.com/users/${GITHUB_ACTOR}/keys")

    if echo "${api_response}" | jq -e 'type == "object" and has("message")' >/dev/null; then
        error_msg=$(echo "${api_response}" | jq -r '.message')
        echo "Error: ${error_msg}"
        return 1
    else
        echo "${api_response}" | jq -r '.[].key'
    fi
}

authorized_keys=$(get_authorized_keys "${GITHUB_ACTOR}")

if [ -n "${authorized_keys}" ]; then
    echo "Configured SSH key(s) for user: ${GITHUB_ACTOR}"
else
    echo "Error: User '${GITHUB_ACTOR}' is not authorized to access this debug session."
    exit 1
fi

if [ "${TUNNEL}" != "true" ]; then
    echo "SSH tunneling is disabled. Set enable-tunnel: true to enable remote access."
    echo "Local SSH server would be available on localhost:2222 if this were a local environment."
    exit 0
fi

echo "SSH tunneling enabled. Setting up remote access..."

EXTERNAL_DEPS="curl jq ssh-keygen"

for dep in $EXTERNAL_DEPS; do
    if ! command -v "${dep}" > /dev/null 2>&1; then
       echo "Command ${dep} not installed on the system!" >&2
       exit 1
    fi
done

cd "$GITHUB_ACTION_PATH"

bashrc_path=$(pwd)/bashrc

# Source `bashrc` to auto start tmux on SSH login.
if ! grep -q "${bashrc_path}" ~/.bash_profile; then
    echo >> ~/.bash_profile # On macOS runner there's no newline at the end of the file
    echo "source \"${bashrc_path}\"" >> ~/.bash_profile
fi

OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

if [ "${ARCH}" = "x86_64" ]; then
    ARCH="amd64"
elif [ "${ARCH}" = "aarch64" ]; then
    ARCH="arm64"
fi

if [ "${OS}" = "darwin" ] && ! command -v tmux > /dev/null 2>&1; then
    echo "Installing tmux..."
    brew install tmux
fi

if [ "$OS" = "darwin" ]; then
    cloudflared_url="https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-${OS}-${ARCH}.tgz"
    echo "Downloading \`cloudflared\` from <$cloudflared_url>..."
    curl --location --silent --output cloudflared.tgz "${cloudflared_url}"
    tar xf cloudflared.tgz
    rm cloudflared.tgz
fi

chmod +x cloudflared

echo "Setting up SSH key for authorized user: ${GITHUB_ACTOR}"
echo "${authorized_keys}" > authorized_keys

echo 'Creating SSH server key...'
ssh-keygen -q -f ssh_host_rsa_key -N ''

echo 'Creating SSH server config...'
sed "s,\$PWD,${PWD},;s,\$USER,${USER}," sshd_config.template > sshd_config

echo 'Starting SSH server...'
/usr/sbin/sshd -f sshd_config -D &
sshd_pid=$!

echo "SSH server started successfully (PID: ${sshd_pid})"

echo 'Starting tmux session...'
(cd "${GITHUB_WORKSPACE}" && tmux new-session -d -s debug)

mkdir ~/.cloudflared
CLEAN_TUNNEL_CERT=$(printf '%s\n' "${CLOUDFLARE_TUNNEL_CERT}" | tr -d '\r' | sed '/^[[:space:]]*$/d')

echo "${CLEAN_TUNNEL_CERT}" > ~/.cloudflared/cert.pem

CLEAN_USER_CA_CERT=$(printf '%s\n' "${CLOUDFLARE_USER_CA_CERT}" | tr -d '\r' | sed '/^[[:space:]]*$/d')

echo "${CLEAN_USER_CA_CERT}" | sudo tee /etc/ssh/ca.pub > /dev/null
sudo chmod 644 /etc/ssh/ca.pub

random_suffix=$(openssl rand -hex 5 | cut -c1-10)
tunnel_name="${GITHUB_SHA}-${GITHUB_RUN_ID}-${random_suffix}"
tunnel_url="${tunnel_name}.${CLOUDFLARE_TUNNEL_HOSTNAME}"

if ./cloudflared tunnel list | grep -q "${tunnel_name}"; then
    echo "Deleting existing tunnel: ${tunnel_name}"
    ./cloudflared tunnel delete ${tunnel_name}
fi

echo "Creating new cloudflare tunnel: ${tunnel_name}"
./cloudflared tunnel create ${tunnel_name}

credentials_file=$(find ~/.cloudflared -name "*.json" | head -n 1)
if [ -z "${credentials_file}" ]; then
    echo "Error: Could not find tunnel credentials file"
    exit 1
fi

echo "Found credentials file: ${credentials_file}"

echo 'Creating tunnel configuration...'
cat > tunnel_config.yml << EOF
tunnel: ${tunnel_name}
credentials-file: ${credentials_file}

ingress:
  - hostname: ${tunnel_url}
    service: ssh://localhost:2222
  - service: http_status:404
EOF

echo 'Setting up DNS routing for tunnel...'
./cloudflared tunnel route dns ${tunnel_name} ${tunnel_url}

echo 'Running cloudflare tunnel...'
./cloudflared tunnel --no-autoupdate --config tunnel_config.yml run 2>&1 | tee cloudflared.log | sed -u 's/^/cloudflared: /' &
cloudflared_pid=$!

public_key=$(cut -d' ' -f1,2 < ssh_host_rsa_key.pub)

(
    echo '    '
    echo '    '
    echo '🔗 SSH Debug Session Ready!'
    echo '━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━'
    echo '    '
    echo '📋 Copy and run this command to connect:'
    echo '    '
    echo "ssh-keygen -R action-ssh-debug && echo 'action-ssh-debug ${public_key}' >> ~/.ssh/known_hosts && ssh -o ProxyCommand='cloudflared access tcp --hostname ${tunnel_url}' runner@action-ssh-debug"
    echo '    '
    echo "⏰ Session expires automatically in ${TIMEOUT} seconds"
    echo '━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━'
    echo '    '
    echo '    '
) | cat

echo 'Starting SSH session in background...'
./ssh-session.sh "${sshd_pid}" "${cloudflared_pid}" "${TIMEOUT}" "${tunnel_name}" &

echo 'SSH session is running in background. GitHub Action will continue.'
echo 'Session will auto-cleanup after timeout or when processes end.'
