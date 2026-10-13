# Alioth recovery compatibility

The platform is Android 16 AERA userspace. Android 17 ROM/device changes are
included only where their kernel, HAL interfaces and recovery dependencies
remain compatible. Framework-only camera, audio, telephony and ROM startup
services are outside recovery.

## Hardware and boot

Alioth/aliothin use recovery-as-boot, Boot header 3, shipping API 30, dynamic
partitions and virtual A/B. Boot metadata is Android 17 / 2026-10 for the target
installed ROM. The coupled maintained PBRP prebuilts originate at
d77b61563cd890bf3a227a2fa9fc41fdabc8bde8; kernel revision
a67e83add1fa9398814db8451f91347c7c2d09ed. Exact artifact hashes are recorded in
prebuilt/SHA256SUMS. Temporary boot uses installed vendor_boot DTB and DTBO.

The native recovery root preserves `/etc -> /system/etc` and Android 16-generated
runtime configuration. ROM-only bootstrap/linker and Android 17 HIDL conventions
must not replace Android 16 recovery implementations. Wrapped-key metadata FBE,
QTI keymaster/gatekeeper services and current normalization retain the Alioth
kernel ABI. Original source history and license notices are preserved.

## Services

Health, graphics allocation and supplicant use system-installed recovery service
domains. Wi-Fi startup loads the built-in CNSS/QCA6390 driver through native init,
with read-only modem firmware, QRTR discovery and a WLAN-only DMS MAC provider.
Factory identity files are never rewritten. Supplicant configuration is copied
into RAM before starting the service. USB uses ConfigFS/FunctionFS readiness;
haptics use the AW8697 evdev FF_CONSTANT interface instead of assuming an
I2C bus number. The labeled activation nodes remain available for diagnostics.

Policy build checks and neverallows remain enabled. The requested permissive
runtime mode remains explicit until enforcing device acceptance is established.
Local allow-rule audits are not evidence of full enforcing behavior.

## Display

Android density 440 matches Evo. AERA's adaptive 1440-wide canvas retains scene
geometry; text 125% and controls 150% are independent to balance Alioth readability.
The configured panel mode and LVGL refresh timer request 120 Hz. A checked
cold modeset completes the Qualcomm splash handoff before applying an explicit
nonpreferred refresh override. Release reports record measured frame delivery.
GPU entry points are resolved dynamically, and a functional software fallback
keeps recovery bootable when vendor GPU initialization fails.

## Source integration and validation

Latest original recovery-side updates are integrated before reapplying Alioth
hardware, layout and durable interrupted-restore fixes. AERA's native transaction
journal and boot guard preserve retryable App Vault registration after an
interrupted restore. Transaction staging is part of the implementation, not a
bring-up substitute. Obsolete PBRP patch replay, alternate packaging tools and
terminal-only diagnostic services are removed.

Use the published manifest and per-release verification report for exact
source revisions, local checks and device results. Device acceptance must cover
startup, touch, brightness, decryption and networking before publication.
