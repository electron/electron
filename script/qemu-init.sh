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

echo "Setting up machine-id for D-Bus"
cat /proc/sys/kernel/random/uuid | tr -d '-' > /etc/machine-id

echo "Setting system clock"
date -s "$(cat /host-time)"


export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
export XDG_RUNTIME_DIR=/run/user/0

echo "Starting entrypoint"
echo "System: $(uname -s) $(uname -r) $(uname -m), page size: $(getconf PAGESIZE) bytes"
sudo chown -R builduser:builduser /root/src
runuser -u builduser -- xvfb-run /root/src/out/Default/electron --version
EXIT_CODE=$?
echo "Test execution finished with exit code $EXIT_CODE"
echo $EXIT_CODE > /exit-code
sync

echo "Powering off"
sudo shutdown -h now
