# AERA Recovery for Xiaomi Alioth

Android 16 recovery userspace for POCO F3 / Redmi K40 (alioth) and Mi 11X
(aliothin), using the maintained Linux 4.19 kernel and Android 17-compatible
wrapped-key decryption services. Device-specific source is independent of Evo.

## Build

```sh
repo init -u https://github.com/AERA-Recovery-Alioth/android_manifest.git -b aera-16.0
repo sync -c --no-clone-bundle --no-tags -j8
source build/envsetup.sh
export AERA_BUILD_TYPE=Alpha AERA_USE_BUSYBOX_BINARY=1
lunch twrp_alioth-bp2a-user
m -j8 adbd bootimage sepolicy_neverallows sepolicy_test sepolicy_dev_type_test
```

Build in a separate checkout. Use AERA's native image/installer packaging.
Alioth uses recovery-as-boot: `out/target/product/alioth/boot.img` is the Boot
image. Temporary testing uses `fastboot boot`; no placeholder vendor_boot image
is distributed. Boot metadata is Android 17 / October 2026 and must match the
installed ROM for decryption. The maintained kernel hashes are in
`prebuilt/SHA256SUMS`; original authorship and licenses remain intact.

## Device integration

- QCA6390/CNSS, QRTR and recovery-local factory WLAN identity service.
- Factory MAC read from persist mounted read-only without journal replay;
  firmware preparation and supplicant configuration stay in RAM.
- Native ConfigFS/FunctionFS USB/MTP, AW8697 input force-feedback haptics and PM8150 RTC/LED paths.
- Android 16 AIDL Health service and Adreno 650 native-buffer allocation.
- Native recovery snapuserd and virtual A/B metadata support.
- Density 440, spacious adaptive UI, text 125% and independently scaled controls 150%.

See AERA_COMPATIBILITY.md for compatibility decisions. Runtime acceptance and
frame-rate measurements belong to the release verification report, not source
configuration claims. Recovery currently retains the user's requested permissive
mode; enforcing support must be established by runtime tests.

## Local validation

After building, run `python3 device/xiaomi/alioth/tools/audit-recovery-drivers.py`.
It validates init/imports, staged firmware and service files, built-in kernel
features and compiled policy permissions without accessing the phone. The
recovery core's CMake tests cover layouts, transaction recovery and hardware
input behavior. Never disable Android policy checks to produce a build.
