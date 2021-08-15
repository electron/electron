#!/usr/bin/env bash

USERNAME="builduser"
VNC_PASSWORD="builduser"
INSTALL_NOVNC="true"

NOVNC_VERSION=1.2.0
WEBSOCKETIFY_VERSION=0.9.0

package_list="
    tigervnc-standalone-server \
    tigervnc-common \
    fluxbox \
    dbus-x11 \
    x11-utils \
    x11-xserver-utils \
    xdg-utils \
    fbautostart \
    at-spi2-core \
    xterm \
    eterm \
    nautilus\
    mousepad \
    seahorse \
    gnome-icon-theme \
    gnome-keyring \
    libx11-dev \
    libxkbfile-dev \
    libsecret-1-dev \
    libgbm-dev \
    libnotify4 \
    libnss3 \
    libxss1 \
    libasound2 \
    xfonts-base \
    xfonts-terminus \
    fonts-noto \
    fonts-wqy-microhei \
    fonts-droid-fallback \
    htop \
    ncdu \
    curl \
    ca-certificates\
    unzip \
    nano \
    locales"

set -e

# Function to run apt-get if needed
apt_get_update_if_needed()
{
    if [ ! -d "/var/lib/apt/lists" ] || [ "$(ls /var/lib/apt/lists/ | wc -l)" = "0" ]; then
        echo "Running apt-get update..."
        apt-get update
    else
        echo "Skipping apt-get update."
    fi
}

# Checks if packages are installed and installs them if not
check_packages() {
    if ! dpkg -s "$@" > /dev/null 2>&1; then
        apt_get_update_if_needed
        apt-get -y install --no-install-recommends "$@"
    fi
}

# Ensure apt is in non-interactive to avoid prompts
export DEBIAN_FRONTEND=noninteractive

apt_get_update_if_needed

# On older Ubuntu, Tilix is in a PPA. on Debian strech its in backports.
if [[ -z $(apt-cache --names-only search ^tilix$) ]]; then
    . /etc/os-release
    if [ "${ID}" = "ubuntu" ]; then
        apt-get install -y --no-install-recommends apt-transport-https software-properties-common
        add-apt-repository -y ppa:webupd8team/terminix
    elif [ "${VERSION_CODENAME}" = "stretch" ]; then
        echo "deb http://deb.debian.org/debian stretch-backports main" > /etc/apt/sources.list.d/stretch-backports.list
    fi
    apt-get update
    if [[ -z $(apt-cache --names-only search ^tilix$) ]]; then
        echo "(!) WARNING: Tilix not available on ${ID} ${VERSION_CODENAME} architecture $(uname -m). Skipping."
    else
        package_list="${package_list} tilix"
    fi
fi

# Install X11, fluxbox and VS Code dependencies
check_packages ${package_list}

# Install Emoji font if available in distro - Available in Debian 10+, Ubuntu 18.04+
if dpkg-query -W fonts-noto-color-emoji > /dev/null 2>&1 && ! dpkg -s fonts-noto-color-emoji > /dev/null 2>&1; then
    apt-get -y install --no-install-recommends fonts-noto-color-emoji
fi

# Check at least one locale exists
if ! grep -o -E '^\s*en_US.UTF-8\s+UTF-8' /etc/locale.gen > /dev/null; then
    echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen 
    locale-gen
fi

# Install the Cascadia Code fonts - https://github.com/microsoft/cascadia-code
if [ ! -d "/usr/share/fonts/truetype/cascadia" ]; then
    curl -sSL https://github.com/microsoft/cascadia-code/releases/download/v2008.25/CascadiaCode-2008.25.zip -o /tmp/cascadia-fonts.zip
    unzip /tmp/cascadia-fonts.zip -d /tmp/cascadia-fonts
    mkdir -p /usr/share/fonts/truetype/cascadia
    mv /tmp/cascadia-fonts/ttf/* /usr/share/fonts/truetype/cascadia/
    rm -rf /tmp/cascadia-fonts.zip /tmp/cascadia-fonts
fi

# Install noVNC
if [ "${INSTALL_NOVNC}" = "true" ] && [ ! -d "/usr/local/novnc" ]; then
    mkdir -p /usr/local/novnc
    curl -sSL https://github.com/novnc/noVNC/archive/v${NOVNC_VERSION}.zip -o /tmp/novnc-install.zip
    unzip /tmp/novnc-install.zip -d /usr/local/novnc
    cp /usr/local/novnc/noVNC-${NOVNC_VERSION}/vnc.html /usr/local/novnc/noVNC-${NOVNC_VERSION}/index.html
    curl -sSL https://github.com/novnc/websockify/archive/v${WEBSOCKETIFY_VERSION}.zip -o /tmp/websockify-install.zip
    unzip /tmp/websockify-install.zip -d /usr/local/novnc
    ln -s /usr/local/novnc/websockify-${WEBSOCKETIFY_VERSION} /usr/local/novnc/noVNC-${NOVNC_VERSION}/utils/websockify
    rm -f /tmp/websockify-install.zip /tmp/novnc-install.zip

    # noVNC works best with Python 2 right now. Install the right package and use it.
    if  [[ -z $(apt-cache --names-only search '^python2-minimal$') ]]; then
        novnc_python_package="python-minimal"
    else
        novnc_python_package="python2-minimal"
    fi
    # Distros all have python-numpy for python2 right now, but future proof
    if [[ -z $(apt-cache --names-only search '^python2-numpy$') ]]; then
        novnc_numpy_package="python-numpy"
    else
        novnc_numpy_package="python2-numpy"
    fi
    if ! dpkg -s ${novnc_python_package} ${novnc_numpy_package} > /dev/null 2>&1; then
        apt-get -y install --no-install-recommends ${novnc_python_package} ${novnc_numpy_package}
    fi
    sed -i -E 's/^python /python2 /' /usr/local/novnc/websockify-${WEBSOCKETIFY_VERSION}/run
fi 

# Set up folders for scripts and init files
mkdir -p /var/run/dbus /usr/local/etc/electron-dev-containers/ /root/.fluxbox

# Script to change resolution of desktop
tee /usr/local/bin/set-resolution > /dev/null \
<< EOF
#!/bin/bash
RESOLUTION=\${1:-\${VNC_RESOLUTION:-1920x1080}}
DPI=\${2:-\${VNC_DPI:-96}}
IGNORE_ERROR=\${3:-"false"}
if [ -z "\$1" ]; then
    echo -e "**Current Settings **\n"
    xrandr
    echo -n -e "\nEnter new resolution (WIDTHxHEIGHT, blank for \${RESOLUTION}, Ctrl+C to abort).\n> "
    read NEW_RES
    if [ "\${NEW_RES}" != "" ]; then
        RESOLUTION=\${NEW_RES}
    fi
    if ! echo "\${RESOLUTION}" | grep -E '[0-9]+x[0-9]+' > /dev/null; then
        echo -e "\nInvalid resolution format!\n"
        exit 1
    fi
    if [ -z "\$2" ]; then
        echo -n -e "\nEnter new DPI (blank for \${DPI}, Ctrl+C to abort).\n> "
        read NEW_DPI
        if [ "\${NEW_DPI}" != "" ]; then
            DPI=\${NEW_DPI}
        fi
    fi
fi

xrandr --fb \${RESOLUTION} --dpi \${DPI} > /dev/null 2>&1

if [ \$? -ne 0 ] && [ "\${IGNORE_ERROR}" != "true" ]; then 
    echo -e "\nFAILED TO SET RESOLUTION!\n"
    exit 1
fi

echo -e "\nSuccess!\n"
EOF

# Container ENTRYPOINT script
tee /usr/local/share/desktop-init.sh > /dev/null \
<< EOF 
#!/bin/bash

USERNAME=${USERNAME}
LOG=/tmp/container-init.log

# Execute the command it not already running
startInBackgroundIfNotRunning()
{
    log "Starting \$1."
    echo -e "\n** \$(date) **" | sudoIf tee -a /tmp/\$1.log > /dev/null
    if ! pidof \$1 > /dev/null; then
        keepRunningInBackground "\$@"
        while ! pidof \$1 > /dev/null; do
            sleep 1
        done
        log "\$1 started."
    else
        echo "\$1 is already running." | sudoIf tee -a /tmp/\$1.log > /dev/null
        log "\$1 is already running."
    fi
}

# Keep command running in background
keepRunningInBackground()
{
    (\$2 bash -c "while :; do echo [\\\$(date)] Process started.; \$3; echo [\\\$(date)] Process exited!; sleep 5; done 2>&1" | sudoIf tee -a /tmp/\$1.log > /dev/null & echo "\$!" | sudoIf tee /tmp/\$1.pid > /dev/null)
}

# Use sudo to run as root when required
sudoIf()
{
    if [ "\$(id -u)" -ne 0 ]; then
        sudo "\$@"
    else
        "\$@"
    fi
}

# Use sudo to run as non-root user if not already running
sudoUserIf()
{
    if [ "\$(id -u)" -eq 0 ] && [ "\${USERNAME}" != "root" ]; then
        sudo -u \${USERNAME} "\$@"
    else
        "\$@"
    fi
}

# Log messages
log()
{
    echo -e "[\$(date)] \$@" | sudoIf tee -a \$LOG > /dev/null
}

log "** SCRIPT START **"

# Start dbus.
log 'Running "/etc/init.d/dbus start".'
if [ -f "/var/run/dbus/pid" ] && ! pidof dbus-daemon  > /dev/null; then
    sudoIf rm -f /var/run/dbus/pid
fi
sudoIf /etc/init.d/dbus start 2>&1 | sudoIf tee -a /tmp/dbus-daemon-system.log > /dev/null
while ! pidof dbus-daemon > /dev/null; do
    sleep 1
done

# Startup tigervnc server and fluxbox
sudo rm -rf /tmp/.X11-unix /tmp/.X*-lock
mkdir -p /tmp/.X11-unix
sudoIf chmod 1777 /tmp/.X11-unix
sudoIf chown root:\${USERNAME} /tmp/.X11-unix
if [ "\$(echo "\${VNC_RESOLUTION}" | tr -cd 'x' | wc -c)" = "1" ]; then VNC_RESOLUTION=\${VNC_RESOLUTION}x16; fi
startInBackgroundIfNotRunning "Xtigervnc" sudoUserIf "tigervncserver -screen \${DISPLAY:-:1} \${VNC_RESOLUTION:-1440x768x16} -rfbport \${VNC_PORT:-5901} -dpi \${VNC_DPI:-96} -localhost -desktop fluxbox -fg -passwd /usr/local/etc/electron-dev-containers/vnc-passwd"

# Spin up noVNC if installed and not runnning.
if [ -d "/usr/local/novnc" ] && [ "\$(ps -ef | grep /usr/local/novnc/noVNC*/utils/launch.sh | grep -v grep)" = "" ]; then
    keepRunningInBackground "noVNC" sudoIf "/usr/local/novnc/noVNC*/utils/launch.sh --listen \${NOVNC_PORT:-6080} --vnc localhost:\${VNC_PORT:-5901}"
    log "noVNC started."
else
    log "noVNC is already running or not installed."
fi

# Run whatever was passed in
log "Executing \"\$@\"."
exec "\$@"
log "** SCRIPT EXIT **"
EOF

echo "${VNC_PASSWORD}" | vncpasswd -f > /usr/local/etc/electron-dev-containers/vnc-passwd
touch /root/.Xmodmap 
chmod +x /usr/local/share/desktop-init.sh /usr/local/bin/set-resolution

tee /root/.fluxbox/apps > /dev/null \
<<EOF
[transient] (role=GtkFileChooserDialog)
  [Dimensions]	{70% 70%}
  [Position]	(CENTER)	{0 0}
[end]
EOF

tee /root/.fluxbox/init > /dev/null \
<<EOF
session.configVersion:	13
session.menuFile:	~/.fluxbox/menu
session.keyFile: ~/.fluxbox/keys
session.styleFile: /usr/share/fluxbox/styles/qnx-photon
session.screen0.workspaces: 1
session.screen0.workspacewarping: false
session.screen0.toolbar.widthPercent: 100
session.screen0.strftimeFormat: %a %l:%M %p
session.screen0.toolbar.tools: RootMenu, clock, iconbar, systemtray
session.screen0.workspaceNames: One,
EOF

tee /root/.fluxbox/menu > /dev/null \
<<EOF
[begin] (  Application Menu  )
    [exec] (File Manager) { nautilus ~ } <>
    [exec] (Text Editor) { mousepad } <>
    [exec] (Terminal) { tilix -w ~ -e $(readlink -f /proc/$$/exe) -il } <>
    [exec] (Web Browser) { x-www-browser --disable-dev-shm-usage } <>
    [submenu] (System) {}
        [exec] (Set Resolution) { tilix -t "Set Resolution" -e bash /usr/local/bin/set-resolution } <>
        [exec] (Edit Application Menu) { mousepad ~/.fluxbox/menu } <>
        [exec] (Passwords and Keys) { seahorse } <>
        [exec] (Top Processes) { tilix -t "Top" -e htop } <>
        [exec] (Disk Utilization) { tilix -t "Disk Utilization" -e ncdu / } <>
        [exec] (Editres) {editres} <>
        [exec] (Xfontsel) {xfontsel} <>
        [exec] (Xkill) {xkill} <>
        [exec] (Xrefresh) {xrefresh} <>
    [end]
    [config] (Configuration)
    [workspaces] (Workspaces)
[end]
EOF

# Set up vnc user
touch /home/${USERNAME}/.Xmodmap
cp -R /root/.fluxbox /home/${USERNAME}
chown -R ${USERNAME}:${USERNAME} /home/${USERNAME}/.Xmodmap /home/${USERNAME}/.fluxbox
chown ${USERNAME}:root /usr/local/share/desktop-init.sh /usr/local/bin/set-resolution /usr/local/etc/electron-dev-containers/vnc-passwd

echo "Done!"
