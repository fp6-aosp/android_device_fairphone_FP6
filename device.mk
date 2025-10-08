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

# A/B support
PRODUCT_PACKAGES += \
    update_engine \
    update_engine_sideload \
    update_verifier

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

# Soong Namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(DEVICE_PATH)
