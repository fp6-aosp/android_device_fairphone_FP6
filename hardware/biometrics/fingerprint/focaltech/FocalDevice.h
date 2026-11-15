/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

namespace focaltech {

// Mirrors ff_driver_config_t of the kernel driver.
struct DriverConfig {
    uint8_t enableFasync;
    uint8_t pad0[3];
    int32_t gestureKeycode[8];
    uint8_t enableSpidev;
    uint8_t pad1[3];
    int32_t spidevBus;
    int32_t spidevCs;
    int32_t gpio[8];
    int32_t logLevel;
    uint8_t logcatDriver;
    uint8_t pad2[3];
};
static_assert(sizeof(DriverConfig) == 0x58);

// Bits reported by the driver through FF_IOC_GET_EVENT_INFO.
enum DeviceEvent : int {
    DEV_EVENT_INTERRUPT = 0,
    DEV_EVENT_SCREEN_ON = 1,
    DEV_EVENT_SCREEN_OFF = 2,
};

class FocalDevice {
  public:
    using EventCallback = std::function<void(DeviceEvent)>;

    FocalDevice() = default;
    ~FocalDevice();

    int init(const DriverConfig& config, EventCallback callback);
    void free();

    int enableIrq(bool enable);
    int spiClock(bool enable);
    int powerOn(bool enable);
    int hwReset();
    int reportKey(uint32_t code, int32_t value);
    void setVendorInfo(const char* info);
    void acquireWakelock(uint32_t timeoutMs);

    bool irqEnabled() const { return mIrqEnabled; }
    bool spiClockEnabled() const { return mSpiClockEnabled; }
    const std::string& driverVersion() const { return mDriverVersion; }

  private:
    int setResetPin(bool level);
    void pollLoop();

    int mFd = -1;
    int mPipe[2] = {-1, -1};
    std::atomic<bool> mIrqEnabled{false};
    std::atomic<bool> mSpiClockEnabled{false};
    std::string mDriverVersion;
    EventCallback mCallback;
    std::thread mThread;
};

}  // namespace focaltech
