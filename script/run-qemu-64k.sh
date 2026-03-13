#!/bin/sh
set -e

CONTAINER=""
TESTFILES=""
ARGS=""

while [ $# -gt 0 ]; do
	case "$1" in
		--container) CONTAINER="$2"; shift 2 ;;
		--testfiles) TESTFILES="$2"; shift 2 ;;
		--) shift; ARGS="$*"; break ;;
		*) echo "Unknown option: $1"; exit 1 ;;
	esac
done

if [ -z "$CONTAINER" ]; then
	echo "Usage: $0 --container CONTAINER [-- ARGS...]"
	exit 1
fi

echo "Installing QEMU system emulation and tools"
sudo apt-get update && sudo apt-get install -y qemu-system-arm binutils

KERNEL_URL="http://ports.ubuntu.com/ubuntu-ports/pool/main/l/linux/linux-image-unsigned-6.8.0-90-generic-64k_6.8.0-90.91_arm64.deb"
KERNEL_DIR=$(mktemp -d)
ROOTFS_DIR=$(mktemp -d)

# Download kernel and export container filesystem in parallel
echo "Downloading kernel and exporting container filesystem in parallel"
curl -fL "$KERNEL_URL" -o "$KERNEL_DIR/kernel.deb" &
CURL_PID=$!

CONTAINER_ID=$(docker create --platform linux/arm64 "$CONTAINER")
docker export "$CONTAINER_ID" | sudo tar -xf - -C "$ROOTFS_DIR"
docker rm -f "$CONTAINER_ID"

echo "Removing container image to free disk space"
docker rmi "$CONTAINER" || true
docker system prune -f || true

wait $CURL_PID

echo "Extracting kernel"
(cd "$KERNEL_DIR" && ar x kernel.deb && tar xf data.tar*)
VMLINUZ="$KERNEL_DIR/boot/vmlinuz-6.8.0-90-generic-64k"
if [ ! -f "$VMLINUZ" ]; then
	echo "Error: Could not find kernel at $VMLINUZ"
	exit 1
fi

sudo cp -r $TESTFILES "$ROOTFS_DIR/home/builduser"

echo "Storing test arguments and installing init script"
echo "$ARGS" > "$ROOTFS_DIR/test-args"
date -u '+%Y-%m-%d %H:%M:%S' > "$ROOTFS_DIR/host-time"
sudo cp "$TESTFILES/electron/script/qemu-init.sh" "$ROOTFS_DIR/init"
sudo chmod +x "$ROOTFS_DIR/init"

echo "Creating disk image with root filesystem"
df -h
DISK_IMG=$(mktemp)
truncate -s 10G "$DISK_IMG"
sudo mkfs.ext4 -q -d "$ROOTFS_DIR" "$DISK_IMG"
sudo rm -rf "$ROOTFS_DIR"

# Use KVM acceleration if available (ARM64 host can run 64K-page guest via KVM)
if [ -e /dev/kvm ] && [ -w /dev/kvm ]; then
	echo "KVM available, using hardware acceleration"
	ACCEL="-accel kvm -cpu host"
else
	echo "KVM not available, using TCG emulation"
	ACCEL="-accel tcg,thread=multi -cpu max,pauth-impdef=on"
fi

echo "Starting QEMU VM with 64K page size kernel"
timeout 1800 qemu-system-aarch64 \
	-M virt \
	$ACCEL \
	-m 4096 \
	-smp 2 \
	-kernel "$VMLINUZ" \
	-append "console=ttyAMA0 root=/dev/vda rw init=/init net.ifnames=0 panic=1" \
	-drive file="$DISK_IMG",format=raw,if=virtio \
	-virtfs local,path="$TESTFILES",mount_tag=testfiles,security_model=none,id=testfiles \
	-netdev user,id=net0 \
	-device virtio-net-pci,netdev=net0 \
	-nographic \
	-no-reboot \
	|| true

echo "Extracting test results from disk image"
MOUNT_DIR=$(mktemp -d)
sudo mount -o loop "$DISK_IMG" "$MOUNT_DIR"
if [ -f "$MOUNT_DIR/results.xml" ]; then
	cp "$MOUNT_DIR/results.xml" .
fi
EXIT_CODE=$(cat "$MOUNT_DIR/exit-code" 2>/dev/null || echo 1)
sudo umount "$MOUNT_DIR"
exit $EXIT_CODE
