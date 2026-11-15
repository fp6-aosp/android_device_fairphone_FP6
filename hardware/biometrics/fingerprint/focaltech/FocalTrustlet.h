/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hardware/hw_auth_token.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "FocalProtocol.h"

struct QSEECom_handle;

namespace focaltech {

struct TaInfo {
    char strings[3][64];
};

struct DeviceInfo {
    uint8_t raw[kDeviceInfoSize];
};

// Client of the Focaltech QSEE trustlet.
class FocalTrustlet {
  public:
    using SpiClockFn = std::function<int(bool)>;

    FocalTrustlet() = default;
    ~FocalTrustlet() { close(); }

    // trustletPath/keymasterPath are "<dir>/<app>". The SPI clock hook, if
    // set, brackets every trustlet command.
    int open(const std::string& trustletPath, const std::string& keymasterPath,
             SpiClockFn spiClock);
    void close();
    bool isOpen() const { return mHandle != nullptr; }

    int init(TaInfo* info);
    int syncConfig(const std::string& json);
    int syncStatistics();
    int initSpi();
    int freeSpi();
    int probeDevice(bool hwReset, DeviceInfo* info);
    int initDevice(DeviceInfo* info);
    int esdHandle();
    int checkHealth(int32_t mode);
    int configPowerMode(int32_t mode);
    int configWorkMode(int32_t mode);
    int queryEventStatus();
    int queryFingerStatus();
    int captureImage(CaptureCtx* capture);
    int reportEvent(EventMsg* event);
    int updateTemplate(EventMsg* event);
    int saveData(uint32_t flags);

    int preEnroll(uint64_t* challenge);
    int enroll(const hw_auth_token_t& hat, uint32_t* gid, bool flag);
    int postEnroll();
    int getAuthenticatorId(uint64_t* id);
    int invalidateAuthenticatorId(uint64_t* id);
    int cancel();
    // Returns the number of enrolled fingers, or a negative error.
    int enumerate(std::vector<std::pair<uint32_t, uint32_t>>* fingers);
    int remove(uint32_t gid, uint32_t fid);
    int setActiveGroup(uint32_t gid, const std::string& path);
    int authenticate(uint64_t operationId, uint32_t gid);
    int resetLockout(const hw_auth_token_t& hat);

  private:
    // Returns a transport error or the error/result value of the trustlet.
    int call(uint32_t cmd, const void* in, uint32_t inLen, void* out = nullptr,
             uint32_t outCap = 0, uint32_t* outLen = nullptr);
    int callLocked(uint32_t cmd, const void* in, uint32_t inLen, void* out, uint32_t outCap,
                   uint32_t* outLen);
    int provisionKeymasterKey(const std::string& keymasterPath);

    std::mutex mMutex;
    QSEECom_handle* mHandle = nullptr;
    uint8_t* mBuffer = nullptr;
    SpiClockFn mSpiClock;
};

}  // namespace focaltech
