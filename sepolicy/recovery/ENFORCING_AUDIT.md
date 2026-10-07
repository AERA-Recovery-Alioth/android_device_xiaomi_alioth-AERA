# Recovery policy validation — October 7, 2026

Global SELinux Enforcing; only the shared recovery domain remains permissive.
init, ueventd, logd and adbd startup and functioning logcat were device-tested.
Password decryption passed: user confirmation plus
twrp.all.users.decrypted=true. Qualcomm libraries must be in the recovery
linker search path; relying only on LD_LIBRARY_PATH broke secure startup.

Measured rules cover init-to-logd rootfs transition, logd rootfs reads,
init brightness/USB-mode writes, inherited keyring search and ueventd
sys_nice. No blanket audit2allow rules or global permissive setting ship.

Build/neverallow checks, exact permissive-domain check, patch replay, theme
consistency, ZIP CRC and embedded-image hash checks passed. The exact packaged
image passed password unlock, touch, brightness and haptics acceptance;
ADB reported twrp.all.users.decrypted=true and global Enforcing.

Limits: recovery's shared UI/decryption domain is permissive. Fastbootd
flashing, postinstall, live standalone ZIP installation and other credential
types were not revalidated in this cycle. No claim of full enforcement.
No partitions were flashed during this policy validation.

Non-blocking startup audit: init's unused USB-rc backup copy is denied and
an early FunctionFS mount attempts module autoload. USB mtp,adb is functional
with built-in FunctionFS. Neither denial warrants broad rootfs create or
module-loading permission.

Final artifact SHA256:
- recovery_boot.img: eaec69e04b3b27ee2373426384d32c62ea5280cc006904a691df35a27d988379
- recovery.zip: 237eaf34aa05bbbbf3adcfa5ebbae971ff9c3957c8446b6c97f93b9541eb4b0c
