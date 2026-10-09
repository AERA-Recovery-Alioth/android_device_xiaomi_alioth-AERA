#!/system/bin/sh
# Alioth boot installer; this writes the active boot partition, not a RAM-only boot.
PATH=/system/bin:/sbin:/vendor/bin
export PATH
OUTFD="$2"
ZIP="$3"
ui() { echo "ui_print $*" > /proc/self/fd/$OUTFD; echo ui_print > /proc/self/fd/$OUTFD; }
fail() { ui "ERROR: $*"; exit 1; }
[ "$(id -u)" = 0 ] || fail "Recovery root is required."
ALIOTH=0
for PROP in ro.product.device ro.product.vendor.device ro.build.product; do
    case "$(getprop "$PROP")" in alioth|aliothin) ALIOTH=1 ;; esac
done
[ "$ALIOTH" = 1 ] || fail "This package is for POCO F3 / alioth only."
for TOOL in getprop unzip sha256sum dd blockdev head wc cut mktemp sync chmod mkdir cp cmp rm; do
    command -v "$TOOL" >/dev/null || fail "Required recovery tool missing: $TOOL"
done
SLOT=$(getprop ro.boot.slot_suffix)
case "$SLOT" in _a|_b) ;; *) fail "Unknown active slot." ;; esac
BOOT=/dev/block/bootdevice/by-name/boot$SLOT
[ -b "$BOOT" ] || fail "Active boot partition not found."
# Stage only in RAM; persistent storage and decryption are not required.
WORK=$(mktemp -d /tmp/pbrp-installer.XXXXXX) || fail "Cannot allocate staging directory."
unzip -o "$ZIP" recovery_boot.img recovery_boot.sha256 magiskboot -d "$WORK" >/dev/null || fail "Cannot extract payload."
cd "$WORK" || fail "Cannot enter staging directory."
sha256sum -c recovery_boot.sha256 || fail "Payload checksum failed."
chmod 755 magiskboot || fail "Cannot enable repack tool."
PARTSIZE=$(blockdev --getsize64 "$BOOT") || fail "Cannot read boot size."
dd if="$BOOT" of=boot-original.img bs=1048576 || fail "Cannot stage current Boot."
[ "$(wc -c < boot-original.img)" = "$PARTSIZE" ] || fail "Incomplete current Boot."
mkdir original recovery verify || fail "Cannot create repack directories."
(cd original && ../magiskboot unpack -h -n ../boot-original.img) || fail "Cannot unpack current Boot."
(cd recovery && ../magiskboot unpack -h -n ../recovery_boot.img) || fail "Cannot unpack recovery."
[ -s recovery/ramdisk.cpio ] || fail "Recovery ramdisk is missing."
cp recovery/ramdisk.cpio original/ramdisk.cpio || fail "Cannot replace ramdisk."
(cd original && ../magiskboot repack ../boot-original.img) || fail "Cannot repack current Boot."
(cd verify && ../magiskboot unpack -h -n ../original/new-boot.img) || fail "Cannot verify repacked Boot."
cmp original/kernel verify/kernel || fail "Kernel preservation check failed."
cmp original/header verify/header || fail "Boot header preservation check failed."
SIZE=$(wc -c < original/new-boot.img)
[ "$SIZE" -gt 0 ] && [ "$SIZE" -le "$PARTSIZE" ] || fail "Repacked Boot exceeds partition size."
ui "Only the shared recovery domain is permissive; infrastructure domains enforce."
ui "Installing recovery ramdisk into active boot$SLOT; current kernel is preserved."
ui "No persistent backup is created. Reinstall Magisk afterward."
dd if=original/new-boot.img of="$BOOT" bs=4096 conv=fsync || fail "Boot write failed; original is retained at $WORK/boot-original.img until reboot."
EXPECTED=$(sha256sum original/new-boot.img | cut -d ' ' -f 1)
ACTUAL=$(head -c "$SIZE" "$BOOT" | sha256sum | cut -d ' ' -f 1)
[ "$EXPECTED" = "$ACTUAL" ] || fail "Boot verification failed; original is retained at $WORK/boot-original.img until reboot."
sync
rm -rf "$WORK"
ui "Verified. Active-slot recovery ramdisk installed; no other partition changed."
exit 0
