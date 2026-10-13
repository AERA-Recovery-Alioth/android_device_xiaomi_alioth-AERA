# Alioth recovery kernel prebuilts

Imported from pbrp-alioth-a17/device_xiaomi_alioth-pbrp, pbrp-a17 branch at
 d77b61563cd890bf3a227a2fa9fc41fdabc8bde8, on2026-10-13.
Kernel source: PocoF3Releases/kernel_xiaomi_sm8250, aosp-17,
commit a67e83add1fa9398814db8451f91347c7c2d09ed.
Release:4.19.325-cip136-st20-perf-ga67e83add1fa.

BoardConfig uses Image, dtbs/alioth.dtb and dtbo.img. SHA256SUMS records the
exact current shipped bytes. Embedded configurations match the previous AERA
Image. All three concatenated kona DTBs match individually; their order differs.
The compact PBRP DTBO payload matches the old partition-sized image prefix,
without importing its trailing AVB data. Temporary boot flashes neither DTBO
nor vendor_boot. No KernelSU integration is added.

Encryption, IncFS, WLAN, AW8697, dm-user and dm-snapshot support remain enabled.
The previous AERA ImageSHA5c2ac99f passed temporary recovery boot and decryption;
that does not certify the newly imported ImageSHA1b5e88a9. This image needs its
own boot verification. Aliothin physical validation remains outstanding.
