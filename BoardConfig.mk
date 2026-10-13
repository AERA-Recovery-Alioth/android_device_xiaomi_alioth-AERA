# SPDX-License-Identifier: Apache-2.0
# Recovery adaptation of the current Alioth Android 17 board configuration.
DEVICE_PATH := device/xiaomi/alioth
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-2a-dotprod
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := cortex-a76
TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_VARIANT := cortex-a76
TARGET_BOARD_PLATFORM := kona
TARGET_BOOTLOADER_BOARD_NAME := kona
TARGET_NO_BOOTLOADER := true
TARGET_PREBUILT_KERNEL := $(DEVICE_PATH)/prebuilt/Image
BOARD_PREBUILT_DTBIMAGE_DIR := $(DEVICE_PATH)/prebuilt/dtbs
BOARD_PREBUILT_DTBOIMAGE := $(DEVICE_PATH)/prebuilt/dtbo.img
BOARD_BOOT_HEADER_VERSION := 3
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_IMAGE_NAME := Image
BOARD_KERNEL_CMDLINE := androidboot.hardware=qcom androidboot.console=ttyMSM0 androidboot.memcg=1 lpm_levels.sleep_disabled=1 msm_rtb.filter=0x237 service_locator.enable=1 androidboot.usbcontroller=a600000.dwc3 swiotlb=2048 loop.max_part=7 cgroup.memory=nokmem,nosocket reboot=panic_warm androidboot.fstab_suffix=qcom androidboot.init_fatal_reboot_target=recovery kpti=off
BOARD_MKBOOTIMG_ARGS += --header_version $(BOARD_BOOT_HEADER_VERSION)
# Bootloader Keymaster must see the installed ROM version, not recovery userspace.
ALIOTH_RECOVERY_OS_VERSION ?= 17.0.0
ALIOTH_RECOVERY_SECURITY_PATCH ?= 2026-10
BOARD_MKBOOTIMG_ARGS += --os_version $(ALIOTH_RECOVERY_OS_VERSION) --os_patch_level $(ALIOTH_RECOVERY_SECURITY_PATCH)
BOARD_BOOTIMAGE_PARTITION_SIZE := 201326592
BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE := 100663296
BOARD_DTBOIMG_PARTITION_SIZE := 33554432
BOARD_FLASH_BLOCK_SIZE := 262144
BOARD_USES_RECOVERY_AS_BOOT := true
BOARD_INCLUDE_DTB_IN_BOOTIMG := true
AB_OTA_UPDATER := true
AB_OTA_PARTITIONS := boot dtbo odm product system system_ext vbmeta vbmeta_system vendor vendor_boot
TARGET_COPY_OUT_VENDOR := vendor
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_USES_METADATA_PARTITION := true
BOARD_USERDATAIMAGE_FILE_SYSTEM_TYPE := f2fs
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/recovery/root/system/etc/recovery.fstab
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TARGET_OTA_ASSERT_DEVICE := alioth,aliothin
TARGET_SCREEN_DENSITY := 440
AERA_THEME := portrait_hdpi
AERA_FRAMERATE := 120
AERA_INCLUDE_CRYPTO := true
AERA_INCLUDE_CRYPTO_FBE := true
AERA_INCLUDE_FBE_METADATA_DECRYPT := true
AERA_USE_FSCRYPT_POLICY := 2
BOARD_USES_QCOM_FBE_DECRYPTION := true
RECOVERY_SDCARD_ON_DATA := true
AERA_INCLUDE_REPACKTOOLS := true
AERA_INCLUDE_RESETPROP := true
AERA_EXTRA_LANGUAGES := true
AERA_INPUT_BLACKLIST := hbtp_vm
AERA_BRIGHTNESS_PATH := /sys/class/backlight/panel0-backlight/brightness
AERA_DEFAULT_BRIGHTNESS := 1200
AERA_MAX_BRIGHTNESS := 2047
AERA_EXCLUDE_DEFAULT_USB_INIT := true
TWRP_INCLUDE_LOGCAT := true
TARGET_USES_LOGD := true
TARGET_USES_MKE2FS := true

BOARD_ROOT_EXTRA_FOLDERS := bluetooth dsp firmware persist
BOARD_SUPER_PARTITION_SIZE := 9126805504
BOARD_SUPER_PARTITION_GROUPS := qti_dynamic_partitions
BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := 9122611200
BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST := system system_ext product vendor odm

# Restore Qualcomm RTC offset from the decrypted time-service state.
TARGET_RECOVERY_QCOM_RTC_FIX := true

# Device labels and permissions for direct recovery hardware access.
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/recovery

# AERA Alioth device configuration.
AERA_DEVICE_VERSION := Alioth
AERA_UI_ADAPTIVE_RESOLUTION := true
# AERA pages use their authored canvas; keep the official adaptive renderer.
AERA_STATUS_H := 124
AERA_STATUS_INDENT_LEFT := 54
AERA_STATUS_INDENT_RIGHT := 54
AERA_SKIP_ADDITIONAL_FSTAB := true

TARGET_BOARD_INFO_FILE := $(DEVICE_PATH)/board-info.txt
BOARD_KERNEL_SEPARATED_DTBO := true
BOARD_USES_QCOM_HARDWARE := true

# Force the recovery ramdisk during fastboot boot on Alioth header-v3 Boot.
BOARD_MKBOOTIMG_ARGS += --cmdline "twrpfastboot=1 printk.devkmsg=on"

# Native AERA PC connection and WLAN stack.
AERA_ENABLE_WLAN := 1
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
WPA_SUPPLICANT_VERSION := VER_0_8_X

# Select the physical panel mode; native UI pacing independently targets 120 Hz.
AERA_DRM_FIXED_REFRESH_RATE := 120

# Match the current Evo allocator/native-handle layout.
TARGET_GRALLOC_HANDLE_HAS_RESERVED_SIZE := true
