# Alioth recovery policy

Device policy is compiled through BOARD_VENDOR_SEPOLICY_DIRS. Hardware
permissions are enclosed in recovery_only and do not alter Android ROM HALs.

Measured hardware labels cover DRM card0, panel backlight, AW8697 haptics,
USB controller mode, RTC, LED and PM8150b battery nodes. Do not grant write
access to all sysfs or copy ROM policy wholesale into recovery.

System-installed Health, graphics allocator and Wi-Fi supplicant have dedicated
service domains. Supplicant runs as wifi with network capabilities and uses a
RAM-only configuration. Factory identity and firmware mounts remain read-only.

Android neverallow, policy and device-type checks remain enabled. Local driver
audits check compiled permissions and staged init/files; they do not prove
complete enforcing behavior. The requested runtime permissive mode remains
explicit in ro.aera.selinux.permissive. Release verification reports record
the tested revision and distinguish local checks from runtime results.
