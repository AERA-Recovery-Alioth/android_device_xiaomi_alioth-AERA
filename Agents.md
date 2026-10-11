# Alioth PBRP development handoff

## Scope and source

- Device tree: pbrp-alioth/device_xiaomi_alioth-pbrp, branch pbrp-a17.
- Recovery root is a PBRP Android 14 checkout; ROM development is in a separate evo checkout. Recovery userspace is intentionally Android 14; target ROM is Android 17.
- Supported identifiers: alioth (POCO F3 / Redmi K40), aliothin (Mi 11X). ROM variants use INDIA/GLOBAL/fallback region detection. Recovery OTA assertions, board-info and installer accept both; no ROM NFC/branding override is necessary for recovery.
- Source/authoritative patch bases and application order: patches/README.md. Final recovery series replay matched the built Git tree exactly; preserve upstream authorship.

## Build and packaging

- User builds only. tools/build-recovery.sh builds bootimage; never flash placeholder vendor_boot.
- Kernel prebuilt comes from the Android 17 alioth kernel; shipped device-tested revision [ac2a025a05e6](https://github.com/PocoF3Releases/kernel_xiaomi_sm8250/commit/ac2a025a05e6f6d9cf12b020c739019fafb33987), Linux 4.19.325-cip136-st20. No KernelSU integration.
- Tracked prebuilt/ contains Image, dtbs/alioth.dtb and dtbo.img with provenance and SHA256SUMS. Main kernel source is maintained in PocoF3Releases/kernel_xiaomi_sm8250, aosp-17.
- Header v3, Boot 201326592 bytes, vendor_boot 100663296 bytes. Recovery-as-boot. Only Boot is distributed.
- tools/package-recovery.py reads the matching ROM ZIP OS/patch metadata, builds the final ramdisk, checks source/staged themes, validates the recovery-only permissive-domain policy and regenerates exact ramdisk-file checksums. It never flashes.
- tools/package-installer.py signs recovery.zip using recovery-local AOSP test-key/signapk tools and validates ZIP CRC, embedded image and payload hashes. Build missing signing tools with `m signapk` in the recovery checkout; packaging must never depend on evo/out. Signed with a public test key, not a private signing identity.
- Direct Ninja sometimes leaves theme resources stale. Refresh/rebuild the theme; packaging fails on source/staged mismatch. Do not silently ship stale XML.

## Runtime capabilities

- UI/touch, brightness, RTC offset correction, timed AW8697 haptics, 120 Hz DRM selection and 120 FPS GUI configuration. No auto-brightness.
- A17 Keymaster/keystore2 FBE metadata/password decryption uses matching boot OS/security patch metadata. User password validation passed. Never request a PIN in chat.
- Virtual A/B property and product inheritance are required for ROM sideload. Verified EvolutionX-17.0-20261004-alioth-12.2-Unofficial.zip installation preserved userdata.
- Magisk v31.0 bundled unmodified; recovery/root/twres/tools/MAGISK_SOURCE.md records upstream asset and SHA256. Installation only on explicit action. No KSU feature or payload.
- Only the shared recovery domain remains permissive. Infrastructure domains init/logd/adbd/fastbootd/postinstall/ueventd enforce. Only sepolicy.recovery has an allowlist; neverallow and Android system/vendor policy checks remain intact. Shell is not permissive. Global Enforcing is not proof all domains enforce.

## Partition and installer behavior

- Mount shows Android filesystems, Data and present USB-OTG. No Persist/EFS/Firmware/Metadata service partition UI.
- Backup/restore targets Boot, Vendor Boot, DTBO and Data; default Boot only. Protected service entries cannot be re-exposed by old backup selections.
- Install browser starts in /data/media/0. Unsupported SD repartition, repair/resize/conversion, manual snapshot/unmap, legacy fixes, kernel replacement and unfinished OTA pages are hidden/guarded.
- Advanced: Install Current Recovery first (confirmed successful on-device), then Install Recovery from Image. Current opens swipe confirmation, image opens picker. Both preserve the existing kernel and update only active Boot. Image-path backup follows the checkbox; no forced persistent backup.
- Flash Current uses the upstream ramdisk file-integrity check; packaging must update hashes for shipped contents, never bypass the check. Packaged integrity check passed across 3500 files.
- recovery.zip stages original Boot only in /tmp, preserves kernel/header, replaces ramdisk, checks size and verifies written bytes. No decryption/OTG/cache backup dependency; no automatic reboot or slot switch. October 9 signed ZIP sideload was device-tested: user confirmed the written-image verification message and successful bundled Magisk reinstall.
- Ramdisk replacement removes the prior Magisk patch; reinstall Magisk afterward.

## Evidence and limits

- User recovery build passed, including Android precompiled policy. Final runtime had recovery/logd processes, userdata decrypted=true and USB mtp,adb. UI/touch/haptics and shortened menus confirmed by user.
- Password decryption, ROM sideload, bundled Magisk install and Boot backup were verified earlier. Current recovery installation completed successfully with image-flash completion on-screen.
- Exact PBRP magiskboot host repack preserved original kernel/header and fitted the 201326592-byte Boot partition. ZIP CRC/image and final ramdisk-file hashes verified.
- Data restore, other credential types, Mi 11X/Redmi K40 physical tests and live image-picker install remain unverified. Standalone ZIP acceptance is recorded below. Do not format/restore userdata merely to test UI.

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

## October 7 recovery device policy

sepolicy/recovery contains exact DRM, panel brightness and AW8697 playback
labels and recovery-only permissions, plus RTC sysfs reads. Policy/build
checks passed; labels and removal of those recovery startup denials were
verified live in a globally permissive diagnostic image. The shared recovery domain remains permissive; a fully enforcing recovery is not validated.
Do not convert property enumeration or diagnostic shell denials to broad
allow rules. See sepolicy/recovery/README.md for remaining validation.

Follow-up: recovery patch 0008 removes O_CREAT/O_TRUNC from the haptic
sysfs writer and checks failed/short writes. Built and temporarily booted;
user confirmed menu navigation, brightness and haptics. The fresh audit
contained no AW8697 control denials after these actions. Compared common
ROM policy: same AW8697 I2C and PM8150 RTC paths; gain/calibration and HAL
permissions are not required for recovery timed playback.
Remaining PBRP-wide denials and enforcing validation are still separate.

## October 7 password startup linker fix

With init enforcing, qseecomd could not load libQSEEComAPI.so despite its
presence under /vendor/lib64. Recovery stopped at the splash while
keystore2 waited for Keymaster. Recovery-only system/etc/ld.config.txt now
includes /vendor/${LIB} in the default search path, avoiding dependence on
LD_LIBRARY_PATH during secure execution. Keep system libraries first.
A temporary boot with this configuration reached the password UI; the user
confirmed it works, and twrp.all.users.decrypted=true was read over ADB.
Global SELinux was Enforcing; recovery itself remains permissive in the
experimental infrastructure policy. This is not full enforcing validation.
No Boot partition was flashed. Published release assets remain unchanged.

## October 7 final infrastructure policy

Vendor-PB patch 0002 removes six infrastructure permissive declarations;
system-sepolicy patch 0002 permits only recovery in user recovery policy.
Recovery patch 0009 adds the init-created logd control socket. Device policy
allows measured init/logd rootfs transitions, logd rootfs reads, init search
of recovery's inherited keyring, ueventd scheduling and exact USB mode and
panel brightness writes. Android policy and neverallow checks remain intact.
The patch series was replayed from all three documented baselines and
matched the built source. User build and packaging passed, including policy
validation, ZIP CRC and embedded image equality. Password unlock passed
on the preceding image with this linker configuration. Final packaged image
was temporarily booted; the user confirmed password unlock, touch, brightness and haptics.
ADB verified twrp.all.users.decrypted=true and global Enforcing.
Only recovery remains permissive: the UI and co-located decryption/Binder
services are not fully enforcing. Fastbootd flashing, postinstall and live
ZIP install were not revalidated in this cycle. Do not overstate coverage.

## October 9 System mount runtime fix

The live UI worked but Magisk/decryption failed after Android System was bound
over /system. ADB sync could read the OS shell, whose /system/bin/linker64
interpreter was missing; command launch returned ENOENT. Recovery patch 0010
prevents that bind/unmount for BOARD_USES_RECOVERY_AS_BOOT, leaving the OS at
/system_root and preserving ramdisk shell/linker/services. User build and package/integrity checks passed. Temporarily booted on slot B:
ADB shell works, ramdisk linker is present, recovery/Keymaster/keystore2 run and
global SELinux is Enforcing. Final October-matched image: password decryption and bundled Magisk installation
confirmed by the user; ADB reads twrp.all.users.decrypted=true and its shell/linker
remain usable after installation.

Packaging may use --metadata-boot with a previously ROM-matched header-v3
recovery image to preserve its OS/security patch when the ROM ZIP is inaccessible.
This cannot establish that the installed ROM matches; use --rom for a new ROM.
Never access ~/evo/out. ~/pbrp-alioth/out is allowed for recovery work.

The first guard restored initial command launch, but password/UI transitions
could still move Android System onto /system. Patch 0011 blocks that switch;
0012 isolates bundled Magisk's own mounts in an updater child namespace. ARM
and ARM64 updater compilation and user Boot rebuild/package passed. Final live
password/Magisk validation passed on the October-matched image; initial guard
alone was insufficient.

Live Keymaster rejected metadata-key upgrade with INVALID_ARGUMENT (-38) when
the Boot header carried September but the installed ROM reported 2026-10-01.
Repackaging with verified October metadata restored the dm-backed Data mount
on slot B. The packaged image uses 17.0.0 / 2026-10; password and bundled Magisk final
acceptance passed (user-confirmed, with decrypted=true read over ADB). Keep header metadata matched to the current ROM.
--security-patch YYYY-MM is permitted only with --metadata-boot for a verified
installed-ROM patch override; --rom remains preferred for a new ROM package.

Initial accepted October 9 candidate (superseded by follow-up packages):
- recovery_boot.img: 2da44881c418359a715b5f6563d0769947311f3feb4771d834e9583578807489
- recovery.zip: f24afbee41f5663f2d3bf0924cc9c722487d1e98c476ce429c52acf4f13a9a61
Temporary boot stayed on slot B. Bundled Magisk installation was confirmed;
parent recovery shell/linker remained intact afterward. Standalone recovery.zip
installation and subsequent Android boot were not retested in this cycle.

## October 9 recovery-local signing and ZIP acceptance

Removed the installer packager dependency on ROM output signing tools. All
signing inputs now come from this recovery checkout: JDK 17, built SignApk/JNI
and AOSP public test keys. `m -j4 signapk` and optimized-Python packaging passed.
ZIP CRC, embedded image equality and payload hashes are explicitly checked.
Corrected the installer policy message and patch guide to reflect only recovery
being permissive. The recovery Boot image remains unchanged.

Updated ZIP SHA256: 10e9903810feef40e045fbfa94e4693d706c9b42adbc139bab7d48fa869b3736.
Active-slot Boot was backed up privately and hash-verified before testing.
Sideload transport became ready after the USB transition; transfer completed.
User confirmed the ZIP written-image verification message and bundled Magisk
reinstall success. ADB afterward confirmed decrypted=true, slot B and intact
linker/shell. No Data format, slot switch or vendor_boot flash was performed.

## October 9 recovery reboot BCB label

Reboot Recovery/fastboot could restart only the recovery process: init failed
to write the bootloader control block because Alioth misc `/dev/block/sda11`
had generic `block_device` context. Live AVCs showed init write denied with
permissive=0. Recovery file_contexts now labels exactly that verified node
`misc_block_device`; the existing platform init rule supplies write access.
No broad block-device allow or permissive-domain change was added. User build,
context validation and recovery-local packaging passed.

Live policy-only deployment preserved the working Magisk ramdisk, kernel and
header; only file_contexts.bin, vendor_file_contexts and ramdisk checksums
changed. Temporary Android boot completed with Magisk 31.0. The verified image
was flashed only to active boot_b. Recovery showed misc_block_device, globally
Enforcing and decrypted=true. `adb reboot recovery` reset kernel uptime and
returned to recovery, with no previous init misc-write denial.

Fastbootd then enumerated, but its 18d1:d00d identity had Windows driver Code 28.
The installed signed Google driver supports 18d1:4ee0, so recovery USB config
now uses that standard generic fastboot identity. This is a source/build fix;
its final live driver compatibility test remains pending device transport.
Windows driver binding needs administrator access; do not weaken recovery
policy to work around a host driver issue.

Final follow-up user build and packaging passed, including theme/policy/integrity,
optimized-Python ZIP CRC, embedded image and payload hashes. Local candidate
(not yet device-tested with the new fastboot USB ID):

- c3a28f66381226995a98bdd2186479750f62f8922e869a604753ad117b64a8fd  recovery_boot.img
- eef82bb2c0b8fec2107875f11e81bbe2197d456494bdc4783296ca8a1ae7a1a2  recovery.zip

## October 11 source checkpoint

Standalone device source: [pbrp-alioth](https://github.com/pbrp-alioth/device_xiaomi_alioth-pbrp/tree/pbrp-a17). All five repositories have only the default branch `pbrp-a17`. Recovery userspace remains Android 14.

| Sibling | Verified implementation commit |
| --- | --- |
| `android_bootable_recovery` | [ef490e810b0a](https://github.com/pbrp-alioth/android_bootable_recovery/commit/ef490e810b0a1d3c12f330242546824327ef9c51) |
| `vendor_pb` | [4f5dc08aafdb](https://github.com/pbrp-alioth/vendor_pb/commit/4f5dc08aafdb70063da058b2bcc8e5e8c04cad05) |
| `android_system_sepolicy` | [edc77e62099a](https://github.com/pbrp-alioth/android_system_sepolicy/commit/edc77e62099aae9c136c1beff24baa34eb70ba14) |
| `android_external_f2fs-tools` | [efd0dc9a3a58](https://github.com/pbrp-alioth/android_external_f2fs-tools/commit/efd0dc9a3a58ad7f4a1d83e6a0fe0028df228238) |

All four patch series replayed from their documented clean bases to exactly the published sibling trees. History consolidation preserved runtime code and imported authorship. Use published sibling branches directly, or apply the patch series to clean bases, never both. Syntax checks and kernel checksums passed. These source checks do not constitute a new recovery build or device test. Final fastboot USB driver compatibility remains pending. There are no release assets in the new standalone repository.
