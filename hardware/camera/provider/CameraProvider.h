/*
 * Copyright (C) 2025 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

using ::aidl::android::hardware::camera::common::VendorTagSection;
using ::aidl::android::hardware::camera::device::ICameraDevice;
using ::aidl::android::hardware::camera::provider::BnCameraProvider;
using ::aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using ::aidl::android::hardware::camera::provider::ConcurrentCameraIdCombination;
using ::aidl::android::hardware::camera::provider::ICameraProviderCallback;

class CameraProvider : public BnCameraProvider {
  public:
    CameraProvider();
    ~CameraProvider() override;

    // Caller must use this method to check if CameraProvider ctor failed
    bool isInitFailed();

    ndk::ScopedAStatus setCallback(
            const std::shared_ptr<ICameraProviderCallback>& in_callback) override;
    ndk::ScopedAStatus getVendorTags(std::vector<VendorTagSection>* _aidl_return) override;
    ndk::ScopedAStatus getCameraIdList(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getCameraDeviceInterface(
            const std::string& in_cameraDeviceName,
            std::shared_ptr<ICameraDevice>* _aidl_return) override;
    ndk::ScopedAStatus notifyDeviceStateChange(int64_t in_deviceState) override;
    ndk::ScopedAStatus getConcurrentCameraIds(
            std::vector<ConcurrentCameraIdCombination>* _aidl_return) override;
    ndk::ScopedAStatus isConcurrentStreamCombinationSupported(
            const std::vector<CameraIdAndStreamCombination>& in_configs,
            bool* _aidl_return) override;

  protected:
    // Reserved space for unknown private fields
    void* reserved[64];
};

} // namespace implementation
} // namespace provider
} // namespace camera
} // namespace hardware
} // namespace android
