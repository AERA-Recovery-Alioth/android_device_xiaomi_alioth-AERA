# Alioth PBRP development handoff

## Scope and source

- Device tree: PocoF3Releases/device_xiaomi_alioth-pbrp, branch pbrp-a17.
- Recovery root is a PBRP Android 14 checkout; ROM development is in a separate evo checkout. Recovery userspace is intentionally Android 14; target ROM is Android 17.
- Supported identifiers: alioth (POCO F3 / Redmi K40), aliothin (Mi 11X). ROM variants use INDIA/GLOBAL/fallback region detection. Recovery OTA assertions, board-info and installer accept both; no ROM NFC/branding override is necessary for recovery.
- Source/authoritative patch bases and application order: patches/README.md. Final recovery series replay matched the built Git tree exactly; preserve upstream authorship.

## Build and packaging

- User builds only. tools/build-recovery.sh builds bootimage; never flash placeholder vendor_boot.
- Kernel prebuilt comes from the Android 17 alioth kernel; previous device-tested revision 321ce2cbf8fbe1533931ba17d8db6ca50e747a39, Linux 4.19.325-cip136-st20. No KernelSU integration.
- Tracked prebuilt/ contains Image, dtbs/alioth.dtb and dtbo.img with provenance and SHA256SUMS. Main kernel source is maintained in PocoF3Releases/kernel_xiaomi_sm8250, aosp-17.
- Header v3, Boot 201326592 bytes, vendor_boot 100663296 bytes. Recovery-as-boot. Only Boot is distributed.
- tools/package-recovery.py reads the matching ROM ZIP OS/patch metadata, builds the final ramdisk, checks source/staged themes, validates the seven-domain policy and regenerates exact ramdisk-file checksums. It never flashes.
- tools/package-installer.py signs recovery.zip using existing AOSP test-key/signapk tools and validates ZIP CRC, embedded image and payload hashes. Signed with a public test key, not a private signing identity.
- Direct Ninja sometimes leaves theme resources stale. Refresh/rebuild the theme; packaging fails on source/staged mismatch. Do not silently ship stale XML.

## Runtime capabilities

- UI/touch, brightness, RTC offset correction, timed AW8697 haptics, 120 Hz DRM selection and 120 FPS GUI configuration. No auto-brightness.
- A17 Keymaster/keystore2 FBE metadata/password decryption uses matching boot OS/security patch metadata. User password validation passed. Never request a PIN in chat.
- Virtual A/B property and product inheritance are required for ROM sideload. Verified EvolutionX-17.0-20261004-alioth-12.2-Unofficial.zip installation preserved userdata.
- Magisk v31.0 bundled unmodified; recovery/root/twres/tools/MAGISK_SOURCE.md records upstream asset and SHA256. Installation only on explicit action. No KSU feature or payload.
- Upstream PBRP seven permissive recovery domains: recovery/init/logd/adbd/fastbootd/postinstall/ueventd. User explicitly approved this recovery policy. Only sepolicy.recovery has an allowlist; neverallow and Android system/vendor policy checks remain intact. Shell is not permissive. Global Enforcing is not proof all domains enforce.

## Partition and installer behavior

- Mount shows Android filesystems, Data and present USB-OTG. No Persist/EFS/Firmware/Metadata service partition UI.
- Backup/restore targets Boot, Vendor Boot, DTBO and Data; default Boot only. Protected service entries cannot be re-exposed by old backup selections.
- Install browser starts in /data/media/0. Unsupported SD repartition, repair/resize/conversion, manual snapshot/unmap, legacy fixes, kernel replacement and unfinished OTA pages are hidden/guarded.
- Advanced: Install Current Recovery first (confirmed successful on-device), then Install Recovery from Image. Current opens swipe confirmation, image opens picker. Both preserve the existing kernel and update only active Boot. Image-path backup follows the checkbox; no forced persistent backup.
- Flash Current uses the upstream ramdisk file-integrity check; packaging must update hashes for shipped contents, never bypass the check. Packaged integrity check passed across 3500 files.
- recovery.zip stages original Boot only in /tmp, preserves kernel/header, replaces ramdisk, checks size and verifies written bytes. No decryption/OTG/cache backup dependency; no automatic reboot or slot switch. Live ZIP install remains untested.
- Ramdisk replacement removes the prior Magisk patch; reinstall Magisk afterward.

## Evidence and limits

- User recovery build passed, including Android precompiled policy. Final runtime had recovery/logd processes, userdata decrypted=true and USB mtp,adb. UI/touch/haptics and shortened menus confirmed by user.
- Password decryption, ROM sideload, bundled Magisk install and Boot backup were verified earlier. Current recovery installation completed successfully with image-flash completion on-screen.
- Exact PBRP magiskboot host repack preserved original kernel/header and fitted the 201326592-byte Boot partition. ZIP CRC/image and final ramdisk-file hashes verified.
- Data restore, other credential types, Mi 11X/Redmi K40 physical tests, live image-picker install and live standalone ZIP install remain unverified. Do not format/restore userdata merely to test UI.

## Windows / WSL device work

- WSL Ubuntu 26.04 for sources/builds. Use Windows platform-tools for USB; tools may be installed under D:/platform-tools. Check actual paths rather than assuming.
- If fastboot transfer times out, reconnect USB and use a Windows-local image copy. This resolved repeated UNC-path transfer failures. fastboot boot is temporary; flashing needs explicit authorization.
- User-build recovery ADB shell is unprivileged; /tmp logs, internal storage and root-only ORS FIFO may be inaccessible. Use recovery UI/MTP rather than weakening permissions.
- adb push into /tmp can fail on fchown; binary shell stdin was truncated on Windows. If a binary transfer is required, use base64 ASCII transport and verify remote SHA256.
- Release checksums live beside artifacts; never treat historical hashes or successful transfer alone as runtime verification.

## October 7 Format Data fix

Kernel prebuilts refreshed from the compiled Android 17 output at
ac2a025a05e6f6d9cf12b020c739019fafb33987, including matching DTB/DTBO.
Runtime uname matches this revision.
Live old recovery Format Data failed with Data EBUSY; /proc/mounts showed
/data/user/0 still bound to the Data filesystem after /sdcard was removed.
Patch recovery/0007 normally unmounts the FBE user-zero alias before Data;
failed teardown aborts rather than lazily detaching before formatting.
The diagnostic image passed unmount/format/remount. Post-format Android
encryption initialization and password decryption need a separate ROM boot.

Validated on Alioth with kernel ac2a025a05e6: Format Data completed with
operation_end status=0. make_f2fs and sload_f2fs returned RC=0; Metadata
mke2fs/e2fsdroid returned RC=0; raw userdata remounted as F2FS.
The sload startup std::bad_cast was fixed by shared C++ runtime linkage
(external/f2fs-tools patch). Preserve this patch in clean source setups.
FBE Format Data intentionally leaves /data/media absent until Android boot.
Full-permissive diagnostic init writes are not included in release images.
Normal-policy image was temporarily booted: getenforce=Enforcing and
sload_f2fs -V succeeded. User repeated Format Data and confirmed Success.
No boot partition was flashed during this diagnostic cycle.

October 7 packaged artifacts (normal policy):
- recovery_boot.img: deca1077d78ce139fabe09af40b38b166216ab33ee0940fb3b1c463e73c2953f
- recovery.zip: fce1374d8dd81a5d8df049fff3a807dfab7102da0c838eb05b2d44921ca052a9
- ZIP CRC and embedded image checks passed; live ZIP installation was not
  repeated in this cycle. Published October 5 assets are not these artifacts.
