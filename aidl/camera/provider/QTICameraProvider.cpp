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

#define LOG_TAG "QTICamPrvdr"

#include "QTICameraProvider.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

ndk::ScopedAStatus QTICameraProvider::getCameraIdList(std::vector<std::string>* _aidl_return) {
    if (_aidl_return == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    ndk::ScopedAStatus status = CameraProvider::getCameraIdList(_aidl_return);

    if (status.isOk()) {
        _aidl_return->resize(3);
    }

    return status;
}

} // namespace implementation
} // namespace provider
} // namespace camera
} // namespace hardware
} // namespace android
