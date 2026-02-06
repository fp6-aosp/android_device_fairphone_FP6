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

DEVICE_PATH := device/fairphone/FP6

## A/B Updates
AB_OTA_UPDATER := true
AB_OTA_PARTITIONS += \
    boot \
    dtbo \
    init_boot \
    pvmfw \
    recovery \
    vbmeta \
    vbmeta_system \
    vendor_boot

## Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-7a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := kryo300

## Audio
AUDIO_FEATURE_ENABLED_DLKM := true
AUDIO_FEATURE_ENABLED_GEF_SUPPORT := true
AUDIO_FEATURE_ENABLED_INSTANCE_ID := true
AUDIO_FEATURE_ENABLED_AGM_HIDL := true
AUDIO_FEATURE_ENABLED_PAL_HIDL := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true
BOARD_SUPPORTS_OPENSOURCE_STHAL := true
TARGET_USES_QCOM_MM_AUDIO := true

## Boot Image
BOARD_BOOT_HEADER_VERSION := 4
BOARD_BOOTCONFIG := \
    androidboot.hardware=qcom \
    androidboot.memcg=1 \
    androidboot.usbcontroller=a600000.dwc3 \
    androidboot.load_modules_parallel=true \
    androidboot.vendor.qspa=true \
    androidboot.console=0 \
    androidboot.hypervisor.protected_vm.supported=true \
    androidboot.hypervisor.version=gunyah
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_CMDLINE := \
    video=vfb:640x400,bpp=32,memsize=3072000 \
    nosoftlockup \
    pstore.compress=none
BOARD_KERNEL_PAGESIZE := 4096
BOARD_INIT_BOOT_HEADER_VERSION := 4
BOARD_INCLUDE_DTB_IN_BOOTIMG := true
BOARD_RAMDISK_USE_LZ4 := true
BOARD_USES_GENERIC_KERNEL_IMAGE := true
BOARD_USES_QCOM_MERGE_DTBS_SCRIPT := true

BOARD_MKBOOTIMG_ARGS := --header_version $(BOARD_BOOT_HEADER_VERSION)
BOARD_MKBOOTIMG_INIT_ARGS := --header_version $(BOARD_INIT_BOOT_HEADER_VERSION)

## Display
TARGET_SCREEN_DENSITY := 440

## DTBO
BOARD_KERNEL_SEPARATED_DTBO := true

## Dynamic Partitions
BOARD_SUPER_PARTITION_SIZE := $(shell echo $$(( 9 * 1024 * 1024 * 1024 )))
BOARD_SUPER_PARTITION_GROUPS := qti_dynamic_partitions
BOARD_QTI_DYNAMIC_PARTITIONS_SIZE := \
    $(shell echo $$(( $(BOARD_SUPER_PARTITION_SIZE) - (4 * 1024 * 1024) )))
BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST := \
    odm \
    product \
    system \
    system_dlkm \
    system_ext \
    vendor \
    vendor_dlkm

AB_OTA_PARTITIONS += \
    $(BOARD_QTI_DYNAMIC_PARTITIONS_PARTITION_LIST)

## Filesystem
TARGET_RO_FILE_SYSTEM_TYPE ?= ext4
BOARD_ODMIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
BOARD_PRODUCTIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
BOARD_SYSTEM_DLKMIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
BOARD_SYSTEM_EXTIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
BOARD_VENDOR_DLKMIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := $(TARGET_RO_FILE_SYSTEM_TYPE)
TARGET_COPY_OUT_ODM := odm
TARGET_COPY_OUT_PRODUCT := product
TARGET_COPY_OUT_SYSTEM_DLKM := system_dlkm
TARGET_COPY_OUT_SYSTEM_EXT := system_ext
TARGET_COPY_OUT_VENDOR := vendor
TARGET_COPY_OUT_VENDOR_DLKM := vendor_dlkm
TARGET_FS_CONFIG_GEN := $(DEVICE_PATH)/configs/filesystem/config.fs
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_USE_F2FS := true

## Kernel
BOARD_KERNEL_IMAGE_NAME := Image
TARGET_KERNEL_ADDITIONAL_FLAGS := \
    BRANCH=android14-6.1 \
    KMI_GENERATION=11 \
    TARGET_BOARD_PLATFORM=volcano
TARGET_KERNEL_CONFIG := \
    gki_defconfig \
    vendor/fps_GKI.config
TARGET_KERNEL_NO_GCC := true
TARGET_KERNEL_SOURCE := kernel/fairphone/sm7635

## Kernel Modules
BOARD_SYSTEM_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/configs/kernel/modules.load.system_dlkm))
BOARD_VENDOR_KERNEL_MODULES_BLOCKLIST_FILE := $(DEVICE_PATH)/configs/kernel/modules.blocklist
BOARD_VENDOR_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/configs/kernel/modules.load.vendor_dlkm))
BOARD_VENDOR_RAMDISK_KERNEL_MODULES_BLOCKLIST_FILE := $(BOARD_VENDOR_KERNEL_MODULES_BLOCKLIST_FILE)
BOARD_VENDOR_RAMDISK_KERNEL_MODULES_LOAD := $(strip $(shell cat $(DEVICE_PATH)/configs/kernel/modules.load.base))
BOARD_VENDOR_RAMDISK_RECOVERY_KERNEL_MODULES_LOAD := \
    $(BOARD_VENDOR_RAMDISK_KERNEL_MODULES_LOAD) \
    $(strip $(shell cat $(DEVICE_PATH)/configs/kernel/modules.load.recovery_extra))
BOOT_KERNEL_MODULES := $(BOARD_VENDOR_RAMDISK_RECOVERY_KERNEL_MODULES_LOAD)
SYSTEM_KERNEL_MODULES := $(BOARD_SYSTEM_KERNEL_MODULES_LOAD)

TARGET_KERNEL_EXT_MODULE_ROOT := kernel/fairphone/sm7635-modules
TARGET_KERNEL_EXT_MODULES := \
    fairphone/emkit \
    fairphone/input/finger \
    fairphone/input/hall_kernel \
    fairphone/input/misc/vl53L1 \
    fairphone/misc/haptic_hv \
    qcom/opensource/mmrm-driver \
    qcom/opensource/mm-drivers/hw_fence \
    qcom/opensource/mm-drivers/msm_ext_display \
    qcom/opensource/mm-drivers/sync_fence \
    qcom/opensource/securemsm-kernel \
    qcom/opensource/audio-kernel \
    qcom/opensource/synx-kernel \
    qcom/opensource/camera-kernel \
    qcom/opensource/datarmnet-ext/mem \
    qcom/opensource/dataipa/drivers/platform/msm \
    qcom/opensource/datarmnet/core \
    qcom/opensource/datarmnet-ext/aps \
    qcom/opensource/datarmnet-ext/offload \
    qcom/opensource/datarmnet-ext/shs \
    qcom/opensource/datarmnet-ext/perf \
    qcom/opensource/datarmnet-ext/perf_tether \
    qcom/opensource/datarmnet-ext/sch \
    qcom/opensource/datarmnet-ext/wlan \
    qcom/opensource/display-drivers/msm \
    qcom/opensource/dsp-kernel \
    qcom/opensource/eva-kernel \
    qcom/opensource/video-driver \
    qcom/opensource/graphics-kernel \
    qcom/opensource/wlan/platform \
    qcom/opensource/wlan/qcacld-3.0/.qca6750 \
    qcom/opensource/wlan/qcacld-3.0/.peach_v2 \
    qcom/opensource/wlan/qcacld-3.0/.wcn6450 \
    qcom/opensource/bt-kernel \
    qcom/opensource/spu-kernel \
    qcom/opensource/mm-sys-kernel/ubwcp \
    qcom/opensource/touch-drivers \
    nxp/opensource/driver \
    samsung_slsi/nfc/driver

## Manifest
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE += \
    $(DEVICE_PATH)/device_framework_matrix.xml \
    hardware/qcom-caf/common/vendor_framework_compatibility_matrix.xml
DEVICE_FRAMEWORK_MANIFEST_FILE := $(DEVICE_PATH)/device_framework_manifest.xml
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/manifest.xml
DEVICE_MATRIX_FILE := hardware/qcom-caf/common/compatibility_matrix.xml

## Partitions
BOARD_FLASH_BLOCK_SIZE := $(shell echo $$(( $(BOARD_KERNEL_PAGESIZE) * 64 )))
BOARD_BOOTIMAGE_PARTITION_SIZE := $(shell echo $$(( 96 * 1024 * 1024 )))
BOARD_DTBOIMG_PARTITION_SIZE := $(shell echo $$(( 30 * 1024 * 1024 )))
BOARD_INIT_BOOT_IMAGE_PARTITION_SIZE := $(shell echo $$(( 8 * 1024 * 1024 )))
BOARD_PVMFWIMAGE_PARTITION_SIZE := $(shell echo $$(( 1 * 1024 * 1024 )))
BOARD_RECOVERYIMAGE_PARTITION_SIZE := $(shell echo $$(( 100 * 1024 * 1024 )))
BOARD_VENDOR_BOOTIMAGE_PARTITION_SIZE := $(shell echo $$(( 96 * 1024 * 1024 )))

BOARD_USES_METADATA_PARTITION := true

## Platform
TARGET_BOARD_PLATFORM := volcano
TARGET_BOOTLOADER_BOARD_NAME := fps

## Recovery
BOARD_EXCLUDE_KERNEL_FROM_RECOVERY_IMAGE := true
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/configs/init/fstab.qcom
TARGET_RECOVERY_PIXEL_FORMAT := RGBX_8888
TARGET_RECOVERY_UI_MARGIN_HEIGHT := 100

## RIL
ENABLE_VENDOR_RIL_SERVICE := true

## Security
BOOT_SECURITY_PATCH := 2025-12-05
VENDOR_SECURITY_PATCH := $(BOOT_SECURITY_PATCH)
VENDOR_SECURITY_PATCH_TIMESTAMP := $(shell date -d 'TZ="GMT" $(VENDOR_SECURITY_PATCH)' +%s)

## SELinux
include device/qcom/sepolicy_vndr/sm8650/SEPolicy.mk

BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor

## Verified Boot
BOARD_AVB_ENABLE := true
ifneq ($(TARGET_AVB_ENABLE),true)
BOARD_AVB_MAKE_VBMETA_IMAGE_ARGS += --flags 3
endif
BOARD_MOVE_GSI_AVB_KEYS_TO_VENDOR_BOOT := true

# Enable chained vbmeta for boot images
BOARD_AVB_BOOT_KEY_PATH := external/avb/test/data/testkey_rsa4096.pem
BOARD_AVB_BOOT_ALGORITHM := SHA256_RSA4096
BOARD_AVB_BOOT_ROLLBACK_INDEX := $(VENDOR_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_BOOT_ROLLBACK_INDEX_LOCATION := 3

# Enable chained vbmeta for init_boot images
BOARD_AVB_INIT_BOOT_KEY_PATH := external/avb/test/data/testkey_rsa4096.pem
BOARD_AVB_INIT_BOOT_ALGORITHM := SHA256_RSA4096
BOARD_AVB_INIT_BOOT_ROLLBACK_INDEX := $(VENDOR_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_INIT_BOOT_ROLLBACK_INDEX_LOCATION := 4

# Enable chainged vbmeta for recovery
BOARD_AVB_RECOVERY_KEY_PATH := external/avb/test/data/testkey_rsa4096.pem
BOARD_AVB_RECOVERY_ALGORITHM := SHA256_RSA4096
BOARD_AVB_RECOVERY_ROLLBACK_INDEX := 1
BOARD_AVB_RECOVERY_ROLLBACK_INDEX_LOCATION := 1

# Enable chain partition for system.
BOARD_AVB_VBMETA_SYSTEM := pvmfw product system system_ext
BOARD_AVB_VBMETA_SYSTEM_KEY_PATH := external/avb/test/data/testkey_rsa4096.pem
BOARD_AVB_VBMETA_SYSTEM_ALGORITHM := SHA256_RSA4096
ifneq ($(TARGET_AVB_ENABLE),true)
BOARD_AVB_MAKE_VBMETA_SYSTEM_IMAGE_ARGS += --flags 3
endif
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX := $(VENDOR_SECURITY_PATCH_TIMESTAMP)
BOARD_AVB_VBMETA_SYSTEM_ROLLBACK_INDEX_LOCATION := 2

## WiFi
BOARD_WLAN_DEVICE := qcwcn
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := //hardware/qcom-caf/wlan/qcwcn:lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := //hardware/qcom-caf/wlan/qcwcn:lib_driver_cmd_$(BOARD_WLAN_DEVICE)
WIFI_FEATURE_HOSTAPD_11AX := true
WIFI_HIDL_FEATURE_AWARE := true
WIFI_HIDL_FEATURE_DUAL_INTERFACE := true
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
WPA_SUPPLICANT_VERSION := VER_0_8_X

# Add common definitions for Qualcomm
include hardware/qcom-caf/common/BoardConfigQcom.mk

# Inherit proprietary vendor configuration
include vendor/fairphone/FP6/BoardConfigVendor.mk

# Inherit Lineage configuration
ifneq ($(LINEAGE_BUILD),)
include $(DEVICE_PATH)/BoardConfigLineage.mk
endif
