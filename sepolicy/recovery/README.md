# Alioth recovery policy

Device policy is compiled through BOARD_VENDOR_SEPOLICY_DIRS. Hardware
permissions are enclosed in recovery_only: they grant nothing to Android HALs.

Validated labels on Alioth:
- /dev/dri/card0: vendor_alioth_recovery_drm_device (DRM UI)
- panel0-backlight/brightness: vendor_alioth_recovery_backlight
- AW8697 activate, activate_mode, duration: vendor_alioth_recovery_haptics

The physical sysfs paths were measured on kernel ac2a025a05e6. Do not grant
write access to all sysfs or copy ROM SELinux policy wholesale into recovery.
Gain/calibration/register interfaces are deliberately not exposed here.

Build and neverallow checks passed. In a temporary globally permissive audit,
the labels applied and previous recovery DRM/backlight/RTC startup denials
were absent. Physical menu navigation, brightness and haptics passed user testing
with patch 0008; credential decryption with these labels remains pending. Shell diagnostic denials are not production
requirements.

This is targeted device policy, not a fully enforcing PBRP conversion.
PBRP's seven permissive service domains are retained. General property
enumeration, co-located services, APEX/filesystem operations and installation
need a separate full enforcing-policy audit. Do not claim they were solved.
