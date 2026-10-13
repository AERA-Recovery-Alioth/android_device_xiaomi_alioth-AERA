# SPDX-License-Identifier: Apache-2.0
LOCAL_PATH := device/xiaomi/alioth
PRODUCT_SHIPPING_API_LEVEL := 30
PRODUCT_USE_DYNAMIC_PARTITIONS := true
$(call inherit-product, $(SRC_TARGET_DIR)/product/virtual_ab_ota.mk)
# Recovery prop.default must expose this even without mounting vendor.
PRODUCT_PROPERTY_OVERRIDES += ro.virtual_ab.enabled=true
PRODUCT_BUILD_SUPER_PARTITION := false
PRODUCT_PACKAGES += qcom_decrypt qcom_decrypt_fbe


# Minimal recovery manifest omits non-recovery framework/test modules.
ALLOW_MISSING_DEPENDENCIES := true

PRODUCT_PACKAGES += alioth_recovery_0 alioth_recovery_1 alioth_recovery_2 alioth_recovery_3 alioth_recovery_4 alioth_recovery_5 alioth_recovery_6 alioth_recovery_7 alioth_recovery_8 alioth_recovery_9 alioth_recovery_10 alioth_recovery_11 alioth_recovery_12 alioth_recovery_13 alioth_recovery_14 alioth_recovery_15 alioth_recovery_16 alioth_recovery_17 alioth_recovery_18 alioth_recovery_19 alioth_recovery_20 alioth_recovery_21 alioth_recovery_22 alioth_recovery_23 alioth_recovery_24 alioth_recovery_25 alioth_recovery_26 alioth_recovery_27 alioth_recovery_28 alioth_recovery_29 alioth_recovery_30 alioth_recovery_31
# Recovery copies recovery/root as the private recovery overlay.

# Recovery-library inclusion/relink convention.
PRODUCT_SOONG_NAMESPACES += vendor/qcom/opensource/commonsys-intf/display
TARGET_RECOVERY_DEVICE_MODULES += android.system.suspend-V1-ndk android.hardware.gatekeeper@1.0 android.hardware.keymaster@4.0 android.hardware.keymaster@4.1 android.system.wifi.keystore@1.0 libion libxml2 libdisplayconfig.qti vendor.display.config@1.0 vendor.display.config@2.0
TW_RECOVERY_ADDITIONAL_RELINK_LIBRARY_FILES += $(TARGET_OUT_SHARED_LIBRARIES)/libion.so $(TARGET_OUT_SHARED_LIBRARIES)/libxml2.so $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/libdisplayconfig.qti.so $(TARGET_OUT_SYSTEM_EXT_SHARED_LIBRARIES)/vendor.display.config@1.0.so $(TARGET_OUT_SYSTEM_EXT_SHARED_LIBRARIES)/vendor.display.config@2.0.so

TW_RECOVERY_ADDITIONAL_RELINK_LIBRARY_FILES += $(TARGET_OUT_SHARED_LIBRARIES)/android.system.suspend-V1-ndk.so

# Alioth UFS slot control, following the maintained OrangeFox device tree.
PRODUCT_SOONG_NAMESPACES += device/xiaomi/alioth hardware/qcom/bootctrl hardware/qcom-caf/bootctrl
PRODUCT_PACKAGES += bootctrl.kona.recovery android.hardware.boot@1.1-impl-qti.recovery
PRODUCT_PROPERTY_OVERRIDES += vendor.gatekeeper.disable_spu=true vendor.usb.use_ffs_mtp=1

# Android 17 Alioth Health fix; unchanged QTI kernel current ABI.
PRODUCT_PACKAGES += android.hardware.health-service.alioth_recovery

PRODUCT_PACKAGES += libcld80211_alioth_recovery

# Alioth readability profile, separate from the Evo-matched Android DPI.
PRODUCT_PROPERTY_OVERRIDES += ro.aera.ui.interface_size=3 ro.aera.ui.profile_version=3 ro.aera.ui.font_scale=125 ro.aera.ui.control_scale=150

# Android 16 runtime with current Evo Qualcomm allocator sources.
PRODUCT_SOONG_NAMESPACES += hardware/qcom-caf/sm8250/display vendor/qcom/opensource/display
SOONG_CONFIG_NAMESPACES += qtidisplay
SOONG_CONFIG_qtidisplay += default gralloc4
SOONG_CONFIG_qtidisplay_default := true
SOONG_CONFIG_qtidisplay_gralloc4 := true
TARGET_RECOVERY_DEVICE_MODULES += libnativewindow libui vendor.qti.hardware.display.allocator-service android.hardware.graphics.mapper@4.0-impl-qti-display
TW_RECOVERY_ADDITIONAL_RELINK_LIBRARY_FILES += $(TARGET_OUT_SHARED_LIBRARIES)/libnativewindow.so $(TARGET_OUT_SHARED_LIBRARIES)/libui.so
TW_RECOVERY_ADDITIONAL_RELINK_BINARY_FILES += $(TARGET_OUT_VENDOR_EXECUTABLES)/hw/vendor.qti.hardware.display.allocator-service
TW_RECOVERY_ADDITIONAL_RELINK_LIBRARY_FILES += $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/hw/android.hardware.graphics.mapper@4.0-impl-qti-display.so

# Shared-library closure of nativewindow/libui and the Qualcomm mapper.
ALIOTH_GPU_SYSTEM_LIBS := android.hardware.common-V2-ndk libgralloctypes android.hardware.graphics.allocator@2.0 android.hardware.graphics.allocator@3.0 android.hardware.graphics.allocator@4.0 android.hardware.graphics.common@1.0 android.hardware.graphics.common@1.1 android.hardware.graphics.common@1.2 android.hardware.graphics.mapper@2.0 android.hardware.graphics.mapper@2.1 android.hardware.graphics.mapper@3.0 android.hardware.graphics.mapper@4.0 android.hardware.graphics.allocator-V2-ndk android.hardware.graphics.common-V6-ndk
ALIOTH_GPU_VENDOR_LIBS := libgrallocutils libgralloccore libgralloc.qti libqdMetaData vendor.qti.hardware.display.allocator@3.0 vendor.qti.hardware.display.allocator@4.0 vendor.qti.hardware.display.mapper@2.0 vendor.qti.hardware.display.mapper@3.0 vendor.qti.hardware.display.mapper@4.0 vendor.qti.hardware.display.mapperextensions@1.0 vendor.qti.hardware.display.mapperextensions@1.1
TARGET_RECOVERY_DEVICE_MODULES += $(ALIOTH_GPU_SYSTEM_LIBS) $(ALIOTH_GPU_VENDOR_LIBS)
TW_RECOVERY_ADDITIONAL_RELINK_LIBRARY_FILES += $(foreach lib,$(ALIOTH_GPU_SYSTEM_LIBS),$(TARGET_OUT_SHARED_LIBRARIES)/$(lib).so) $(foreach lib,$(ALIOTH_GPU_VENDOR_LIBS),$(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/$(lib).so)

# Legacy QTI HIDL versions are installed in system_ext on Android 16.
TW_RECOVERY_ADDITIONAL_RELINK_LIBRARY_FILES += $(TARGET_OUT_SYSTEM_EXT_SHARED_LIBRARIES)/vendor.qti.hardware.display.allocator@3.0.so $(TARGET_OUT_SYSTEM_EXT_SHARED_LIBRARIES)/vendor.qti.hardware.display.mapper@2.0.so $(TARGET_OUT_SYSTEM_EXT_SHARED_LIBRARIES)/vendor.qti.hardware.display.mapper@3.0.so

# sm8250-common v1 nv_mac provisioning; mounted read-only by recovery init.
PRODUCT_PROPERTY_OVERRIDES += ro.aera.wlan.factory_mac_file=/mnt/vendor/persist/wlan_mac.bin

# Recovery-local DMS WLAN identity provider; imported upstream QRTR/QMI protocol.
PRODUCT_PACKAGES += aera-alioth-wlan-mac

# Android 16 has the same native recovery snapshot daemon as current PBRP.
PRODUCT_PACKAGES += snapuserd.recovery

# Explicit recovery runtime mode requested for this device.
PRODUCT_PROPERTY_OVERRIDES += ro.aera.selinux.permissive=1

# Keep native init and the WLAN backend on the same RAM-local configuration.
PRODUCT_PROPERTY_OVERRIDES += ro.aera.wlan.supplicant_conf=/tmp/recovery/wpa_supplicant.conf

# Device PCM bridge for browser, media and Doom; untrusted UIDs keep no ALSA access.
PRODUCT_PACKAGES += aera-audio-bridge
PRODUCT_PACKAGES += aera-alioth-pd-mapper
