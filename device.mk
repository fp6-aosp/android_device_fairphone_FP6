#
# Copyright (C) 2025 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DEVICE_PATH := device/fairphone/FP6

# Inherit from generic products, most specific first
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit_only.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Setup dalvik vm configs
$(call inherit-product, frameworks/native/build/phone-xhdpi-6144-dalvik-heap.mk)

# Enable virtual A/B
$(call inherit-product, $(SRC_TARGET_DIR)/product/generic_ramdisk.mk)
$(call inherit-product, \
    $(SRC_TARGET_DIR)/product/virtual_ab_ota/vabc_features.mk)

PRODUCT_VENDOR_PROPERTIES += ro.virtual_ab.compression.threads=true
PRODUCT_VIRTUAL_AB_COMPRESSION_METHOD := lz4

# Enable project quotas and casefolding for emulated storage without sdcardfs
$(call inherit-product, $(SRC_TARGET_DIR)/product/emulated_storage.mk)

# Installs gsi keys into ramdisk, to boot a developer GSI with verified boot.
$(call inherit-product, $(SRC_TARGET_DIR)/product/developer_gsi_keys.mk)

# Add common definitions for Qualcomm
$(call inherit-product, hardware/qcom/common/common.mk)

# Get non-open-source specific aspects
$(call inherit-product, vendor/fairphone/FP6/FP6-vendor.mk)

# A/B support
PRODUCT_PACKAGES += \
    checkpoint_gc \
    otapreopt_script \
    update_engine \
    update_engine_sideload \
    update_verifier

AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_system=true \
    POSTINSTALL_PATH_system=system/bin/otapreopt_script \
    FILESYSTEM_TYPE_system=ext4 \
    POSTINSTALL_OPTIONAL_system=true

AB_OTA_POSTINSTALL_CONFIG += \
    RUN_POSTINSTALL_vendor=true \
    POSTINSTALL_PATH_vendor=bin/checkpoint_gc \
    FILESYSTEM_TYPE_vendor=ext4 \
    POSTINSTALL_OPTIONAL_vendor=true

# Audio
PRODUCT_PACKAGES += \
    android.hardware.audio@7.1-impl \
    android.hardware.audio.effect@7.0-impl \
    android.hardware.audio.service \
    android.hardware.bluetooth.audio-impl \
    android.hardware.soundtrigger@2.3-impl

PRODUCT_PACKAGES += \
    audio.bluetooth.default \
    audio.primary.volcano \
    audio.r_submix.default \
    audio.usb.default \
    sound_trigger.primary.volcano

PRODUCT_PACKAGES += \
    audioadsprpcd

PRODUCT_PACKAGES += \
    lib_bt_aptx \
    lib_bt_ble \
    lib_bt_bundle \
    libagm_compress_plugin \
    libagm_mixer_plugin \
    libagm_pcm_plugin \
    libbatterylistener \
    libfmpal \
    libhfp_pal \
    libqcompostprocbundle \
    libqcomvisualizer \
    libqcomvoiceprocessing \
    libvolumelistener

CONFIG_SKU_OUT_DIR := $(TARGET_COPY_OUT_VENDOR)/etc/audio/sku_volcano

PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/audio/audio_policy_configuration.xml:$(CONFIG_SKU_OUT_DIR)/audio_policy_configuration.xml \
    $(DEVICE_PATH)/configs/audio/mixer_paths_volcano_mtp.xml:$(CONFIG_SKU_OUT_DIR)/mixer_paths_volcano_mtp.xml \
    $(DEVICE_PATH)/configs/audio/resourcemanager_volcano_mtp.xml:$(CONFIG_SKU_OUT_DIR)/resourcemanager_volcano_mtp.xml \
    $(DEVICE_PATH)/configs/audio/usecaseKvManager.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usecaseKvManager.xml

CONFIG_HAL_SRC_DIR := hardware/qcom/sm8650/audio/primary-hal/configs/volcano
CONFIG_PAL_SRC_DIR := hardware/qcom/sm8650/audio/pal/configs/volcano

PRODUCT_COPY_FILES += \
    $(CONFIG_HAL_SRC_DIR)/audio_effects.xml:$(CONFIG_SKU_OUT_DIR)/audio_effects.xml \
    $(CONFIG_HAL_SRC_DIR)/microphone_characteristics.xml:$(TARGET_COPY_OUT_VENDOR)/etc/microphone_characteristics.xml \
    $(CONFIG_PAL_SRC_DIR)/card-defs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/card-defs.xml \
    hardware/qcom/sm8650/audio/primary-hal/configs/common/media_codecs_vendor_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_vendor_audio.xml \
    hardware/qcom/sm8650/audio/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.base-arm64.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/c2audio.vendor.base-arm64.policy \
    hardware/qcom/sm8650/audio/primary-hal/configs/common/codec2/service/1.0/c2audio.vendor.ext-arm64.policy:$(TARGET_COPY_OUT_VENDOR)/etc/seccomp_policy/c2audio.vendor.ext-arm64.policy \
    hardware/qcom/sm8650/audio/primary-hal/configs/common/codec2/media_codecs_c2_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_c2_audio.xml

PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml

# BootControl HAL
PRODUCT_PACKAGES += \
    android.hardware.boot-service.qti \
    android.hardware.boot-service.qti.recovery

# Device identifier, this must come after all inclusions
PRODUCT_NAME := FP6
PRODUCT_NAME_FOR_ATTESTATION := FP6
PRODUCT_DEVICE := FP6
PRODUCT_BRAND := Fairphone
PRODUCT_BRAND_FOR_ATTESTATION := Fairphone
PRODUCT_MODEL := Fairphone 6
PRODUCT_MODEL_FOR_ATTESTATION := Fairphone 6
PRODUCT_MANUFACTURER := Fairphone
PRODUCT_SHIPPING_API_LEVEL := 35

PRODUCT_GMS_CLIENTID_BASE := android-fairphone

# Dynamic Partitions
PRODUCT_USE_DYNAMIC_PARTITIONS := true

# fastbootd
PRODUCT_PACKAGES += \
    fastbootd

# Graphics
PRODUCT_PACKAGES += \
    android.hardware.graphics.mapper@4.0-impl-qti-display \
    vendor.qti.hardware.display.allocator-service \
    vendor.qti.hardware.display.composer-service \
    vendor.qti.hardware.display.demura-service

PRODUCT_PACKAGES += \
    init.qti.display_boot.rc \
    init.qti.display_boot.sh

PRODUCT_COPY_FILES += \
    hardware/qcom/sm8650/display/config/snapdragon_color_libs_config.xml:$(TARGET_COPY_OUT_VENDOR)/etc/snapdragon_color_libs_config.xml

# Health
PRODUCT_PACKAGES += \
    android.hardware.health-service.qti \
    android.hardware.health-service.qti_recovery

# HIDL
# TODO(b/330696629) remove this once device can drop HIDL.
PRODUCT_PACKAGES += \
    hwservicemanager \
    android.hidl.allocator@1.0-service

# init
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/init/charger_fw_fstab.qti:$(TARGET_COPY_OUT_VENDOR)/etc/charger_fw_fstab.qti \
    $(DEVICE_PATH)/configs/init/fstab.qcom:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.qcom \
    $(DEVICE_PATH)/configs/init/fstab.qcom:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.qcom \
    $(DEVICE_PATH)/configs/init/init.class_main.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.class_main.sh \
    $(DEVICE_PATH)/configs/init/init.crda.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.crda.sh \
    $(DEVICE_PATH)/configs/init/init.embmssl_server.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.embmssl_server.rc \
    $(DEVICE_PATH)/configs/init/init.kernel.early_boot-memory.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.early_boot-memory.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-memory.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-memory.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano_2_2_1.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano_2_2_1.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano_3_2_1.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano_3_2_1.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano_3_3_1.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano_3_3_1.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano_4_2_1.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano_4_2_1.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano_4_3_0.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano_4_3_0.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot-volcano_default_4_3_1.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot-volcano_default_4_3_1.sh \
    $(DEVICE_PATH)/configs/init/init.kernel.post_boot.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.kernel.post_boot.sh \
    $(DEVICE_PATH)/configs/init/init.mdm.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.mdm.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.class_core.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.class_core.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.coex.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.coex.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.early_boot.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.early_boot.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.efs.sync.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.efs.sync.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.factory.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.qcom.factory.rc \
    $(DEVICE_PATH)/configs/init/init.qcom.post_boot.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.post_boot.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.qcom.rc \
    $(DEVICE_PATH)/configs/init/init.qcom.sdio.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.sdio.sh \
    $(DEVICE_PATH)/configs/init/init.qcom.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qcom.sh \
    $(DEVICE_PATH)/configs/init/init.qlm-service.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.qlm-service.rc \
    $(DEVICE_PATH)/configs/init/init.qti.graphics.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.qti.graphics.rc \
    $(DEVICE_PATH)/configs/init/init.qti.graphics.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.graphics.sh \
    $(DEVICE_PATH)/configs/init/init.qti.kernel.debug-volcano.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.kernel.debug-volcano.sh \
    $(DEVICE_PATH)/configs/init/init.qti.kernel.debug.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.kernel.debug.sh \
    $(DEVICE_PATH)/configs/init/init.qti.kernel.early_debug-volcano.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.kernel.early_debug-volcano.sh \
    $(DEVICE_PATH)/configs/init/init.qti.kernel.early_debug.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.kernel.early_debug.sh \
    $(DEVICE_PATH)/configs/init/init.qti.kernel.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.qti.kernel.rc \
    $(DEVICE_PATH)/configs/init/init.qti.kernel.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.kernel.sh \
    $(DEVICE_PATH)/configs/init/init.qti.ufs.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.qti.ufs.rc \
    $(DEVICE_PATH)/configs/init/init.qti.write.sh:$(TARGET_COPY_OUT_VENDOR)/bin/init.qti.write.sh \
    $(DEVICE_PATH)/configs/init/init.recovery.qcom.rc:$(TARGET_COPY_OUT_RECOVERY)/root/init.recovery.qcom.rc \
    $(DEVICE_PATH)/configs/init/init.target.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/hw/init.target.rc \
    $(DEVICE_PATH)/configs/init/system_dlkm_modprobe.sh:$(TARGET_COPY_OUT_VENDOR)/bin/system_dlkm_modprobe.sh \
    $(DEVICE_PATH)/configs/init/ueventd-odm.rc:$(TARGET_COPY_OUT_ODM)/etc/ueventd.rc \
    $(DEVICE_PATH)/configs/init/ueventd.qcom.rc:$(TARGET_COPY_OUT_VENDOR)/etc/ueventd.rc \
    $(DEVICE_PATH)/configs/init/vendor_modprobe.sh:$(TARGET_COPY_OUT_VENDOR)/bin/vendor_modprobe.sh

# Kernel
PRODUCT_ENABLE_UFFD_GC := true
PRODUCT_VENDOR_KERNEL_HEADERS := $(DEVICE_PATH)/kernel-headers

# Mountpoints
PRODUCT_PACKAGES += \
    vendor_bt_firmware_mountpoint \
    vendor_dsp_mountpoint \
    vendor_firmware_mnt_mountpoint \
    vendor_vm-system_mountpoint

# Overlays
PRODUCT_PACKAGES += \
    CellBroadcastReceiverResCommon_Vendor \
    FrameworksResCommon_Vendor \
    FrameworksResTarget_Vendor \
    SecureElementResTarget_Vendor \
    SettingsResCommon_Vendor \
    SystemUIResCommon_Vendor \
    TelecommResCommon_Vendor \
    TelephonyResCommon_Vendor \
    UwbResCommon_Vendor \
    WifiResCommon_Vendor \
    WifiResTarget

# Permissions
PRODUCT_PACKAGES += \
    android.hardware.audio.low_latency.prebuilt.xml \
    android.hardware.bluetooth.prebuilt.xml \
    android.hardware.bluetooth_le.prebuilt.xml \
    android.hardware.camera.flash-autofocus.prebuilt.xml \
    android.hardware.camera.front.prebuilt.xml \
    android.hardware.camera.full.prebuilt.xml \
    android.hardware.camera.raw.prebuilt.xml \
    android.hardware.fingerprint.prebuilt.xml \
    android.hardware.hardware_keystore_V3.xml \
    android.hardware.location.gps.prebuilt.xml \
    android.hardware.nfc.hce.prebuilt.xml \
    android.hardware.nfc.hcef.prebuilt.xml \
    android.hardware.nfc.uicc.prebuilt.xml \
    android.hardware.nfc.prebuilt.xml \
    android.hardware.se.omapi.uicc.prebuilt.xml \
    android.hardware.sensor.accelerometer.prebuilt.xml \
    android.hardware.sensor.barometer.prebuilt.xml \
    android.hardware.sensor.compass.prebuilt.xml \
    android.hardware.sensor.dynamic.head_tracker.prebuilt.xml \
    android.hardware.sensor.gyroscope.prebuilt.xml \
    android.hardware.sensor.light.prebuilt.xml \
    android.hardware.sensor.proximity.prebuilt.xml \
    android.hardware.sensor.stepcounter.prebuilt.xml \
    android.hardware.sensor.stepdetector.prebuilt.xml \
    android.hardware.telephony.gsm.prebuilt.xml \
    android.hardware.telephony.ims.prebuilt.xml \
    android.hardware.usb.accessory.prebuilt.xml \
    android.hardware.usb.host.prebuilt.xml \
    android.hardware.vulkan.compute-0.prebuilt.xml \
    android.hardware.vulkan.level-1.prebuilt.xml \
    android.hardware.vulkan.version-1_3.prebuilt.xml \
    android.hardware.wifi.direct.prebuilt.xml \
    android.hardware.wifi.passpoint.prebuilt.xml \
    android.hardware.wifi.prebuilt.xml \
    android.software.device_id_attestation.prebuilt.xml \
    android.software.ipsec_tunnels.prebuilt.xml \
    android.software.opengles.deqp.level-2023-03-01.prebuilt.xml \
    android.software.sip.voip.prebuilt.xml \
    android.software.verified_boot.prebuilt.xml \
    android.software.vulkan.deqp.level-2023-03-01.prebuilt.xml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.audio.pro.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.pro.xml \
    frameworks/native/data/etc/android.hardware.keystore.app_attest_key.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.keystore.app_attest_key.xml \
    frameworks/native/data/etc/android.hardware.opengles.aep.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.opengles.aep.xml \
    frameworks/native/data/etc/android.hardware.telephony.mbms.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.mbms.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.hardware.wifi.aware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.aware.xml \
    frameworks/native/data/etc/android.hardware.wifi.rtt.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.rtt.xml \
    frameworks/native/data/etc/android.software.midi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.midi.xml

# Power
PRODUCT_PACKAGES += \
    android.hardware.power-service-qti

PRODUCT_COPY_FILES += \
    vendor/qcom/opensource/power/config/volcano/powerhint.xml:$(TARGET_COPY_OUT_VENDOR)/etc/powerhint.xml

# Soong Namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(DEVICE_PATH) \
    hardware/qcom-caf/wlan \
    hardware/qcom/bootctrl \
    hardware/qcom/sm8650/audio/agm \
    hardware/qcom/sm8650/audio/graphservices \
    hardware/qcom/sm8650/audio/pal \
    hardware/qcom/sm8650/audio/primary-hal \
    hardware/qcom/sm8650/display \
    hardware/qcom/sm8650/gps \
    vendor/qcom/opensource/commonsys-intf/display \
    vendor/qcom/opensource/dataservices \
    vendor/qcom/opensource/usb/etc

# Telephony
PRODUCT_PACKAGES += \
    extphonelib \
    extphonelib-product \
    extphonelib.xml \
    extphonelib_product.xml \
    ims-ext-common \
    ims_ext_common.xml \
    qti-telephony-hidl-wrapper \
    qti-telephony-hidl-wrapper-prd \
    qti_telephony_hidl_wrapper.xml \
    qti_telephony_hidl_wrapper_prd.xml \
    qti-telephony-utils \
    qti-telephony-utils-prd \
    qti_telephony_utils.xml \
    qti_telephony_utils_prd.xml \
    telephony-ext

PRODUCT_BOOT_JARS += \
    telephony-ext

# USB
PRODUCT_PACKAGES += \
    android.hardware.usb-service.qti \
    android.hardware.usb.gadget-service.qti

PRODUCT_PACKAGES += \
    init.qcom.usb.rc \
    init.qcom.usb.rc \
    usb_compositions.conf

# Vendor service manager
PRODUCT_PACKAGES += \
    vndservicemanager
