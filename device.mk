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
BOARD_SHIPPING_API_LEVEL := 34

PRODUCT_GMS_CLIENTID_BASE := android-fairphone

# Dynamic Partitions
PRODUCT_USE_DYNAMIC_PARTITIONS := true

# fastbootd
PRODUCT_PACKAGES += \
    fastbootd

# init
PRODUCT_COPY_FILES += \
    $(DEVICE_PATH)/configs/init/fstab.qcom:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.qcom \
    $(DEVICE_PATH)/configs/init/fstab.qcom:$(TARGET_COPY_OUT_VENDOR_RAMDISK)/first_stage_ramdisk/fstab.qcom \
    $(DEVICE_PATH)/configs/init/init.recovery.qcom.rc:$(TARGET_COPY_OUT_RECOVERY)/root/init.recovery.qcom.rc

# Soong Namespaces
PRODUCT_SOONG_NAMESPACES += \
    $(DEVICE_PATH)
