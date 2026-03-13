#!/bin/sh

echo "Mounting essential filesystems"
mount -t proc proc /proc
mount -t sysfs sys /sys
mkdir -p /dev/pts
mount -t devpts devpts /dev/pts
mkdir -p /dev/shm
mount -t tmpfs tmpfs /dev/shm
mount -t tmpfs tmpfs /tmp
chmod 1777 /tmp
mount -t tmpfs tmpfs /run
mkdir -p /run/dbus
mkdir -p /run/user/0
chmod 700 /run/user/0
mount -t tmpfs tmpfs /var/tmp

echo "Setting up hostname and machine-id for D-Bus"
echo "electron-test" > /etc/hostname
hostname electron-test
echo "127.0.0.1 electron-test" >> /etc/hosts
cat /proc/sys/kernel/random/uuid | tr -d '-' > /etc/machine-id

echo "Configuring network"
# QEMU user-mode networking provides DHCP at 10.0.2.2 and DNS at 10.0.2.3
# Find the virtio network interface and configure it via DHCP
for iface in /sys/class/net/*/; do
	iface=$(basename "$iface")
	[ "$iface" = "lo" ] && continue
	ip link set "$iface" up
	# Try dhclient first, fall back to manual configuration
	if command -v dhclient >/dev/null 2>&1; then
		dhclient "$iface"
	else
		ip addr add 10.0.2.15/24 dev "$iface"
		ip route add default via 10.0.2.2
	fi
	break
done
# Configure DNS resolver (QEMU SLIRP DNS forwarder)
echo "nameserver 10.0.2.3" > /etc/resolv.conf

echo "Setting system clock"
date -s "$(cat /host-time)"


export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export XDG_RUNTIME_DIR=/run/user/0

echo "Starting entrypoint"
echo "System: $(uname -s) $(uname -r) $(uname -m), page size: $(getconf PAGESIZE) bytes"
sudo chown -R builduser:builduser /home/builduser
ls -la /home/builduser/src/out/Default/electron
cd /home/builduser/src/electron
node script/yarn.js install --immutable
runuser -u builduser -- xvfb-run script/actions/run-tests.sh script/yarn.js test --skipYarnInstall --runners=main --trace-uncaught --enable-logging --files spec/api-app-spec.ts
EXIT_CODE=$?
echo "Test execution finished with exit code $EXIT_CODE"
echo $EXIT_CODE > /exit-code
sync

echo "Powering off"
# poweroff -f bypasses the init system (this script IS pid 1) and
# directly invokes the reboot() syscall, causing QEMU to exit immediately.
poweroff -f
