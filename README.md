# PitchBlack Recovery — POCO F3 / Redmi K40 / Mi 11X

PBRP 4.0, Android 14 recovery userspace compatible with the tested Android 17
Evolution X build. Supported codenames: **alioth, aliothin**.

[Download the final release](https://github.com/PocoF3Releases/device_xiaomi_alioth-pbrp/releases/tag/pbrp-4.0-20261005).

## Installation

- Temporary launch: `fastboot boot recovery_boot.img`.
- Permanent installation: Advanced → **Install Current Recovery** (tested),
  or **Install Recovery from Image**. The current option appears first.
- `recovery.zip` installs the recovery ramdisk into **active Boot**, retaining
  its kernel/header. It requires neither decrypted storage nor a persistent
  backup. This ZIP path passed file-based checks; live ZIP installation is untested.
- Reinstall Magisk after ramdisk installation. Magisk v31.0 is bundled under
  Tools, with official provenance and checksum alongside the package.

Do not flash the generated placeholder vendor_boot. Neither image nor ZIP
formats userdata. Only the shared recovery domain remains permissive; infrastructure services enforce.
Android policy and neverallow checks remain intact. It is not fully enforcing.

## Verified and remaining checks

Verified: UI/touch, brightness, timed haptics, RTC, 120 Hz selection, MTP,
Boot backup, Android 17 password decryption, Virtual A/B ROM sideload,
Magisk installation and current-recovery ramdisk installation.
Data restore, other credential types and physical Mi 11X/Redmi K40 testing
remain unverified. GUI frame pacing is configured for 120 FPS.

## Rebuild

Use a PBRP Android 14 checkout with this tree at `device/xiaomi/alioth`.
Apply the pinned sibling [patch series](patches/README.md) once to clean bases.
Provide the matching Android 17 kernel/DTB/DTBO in ignored `prebuilt/`:
`Image`, `dtbs/`, and `dtbo.img`; kernel sources are maintained separately.
Run `device/xiaomi/alioth/tools/build-recovery.sh` (user build, Boot only).

Package against the matching installed ROM ZIP to keep Keymaster OS/security
patch metadata correct:

```sh
python3 device/xiaomi/alioth/tools/package-recovery.py --rom /path/to/ROM.zip
python3 device/xiaomi/alioth/tools/package-installer.py
```

Deliverables: `out/release-candidate/recovery_boot.img`, `recovery.zip` and
`SHA256SUMS`. ZIP signing uses the public AOSP test key, not a private release key.
Development handoff: [Agents.md](Agents.md).
