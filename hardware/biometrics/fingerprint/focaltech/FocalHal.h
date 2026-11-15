/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hardware/fingerprint.h>
#include <hardware/hw_auth_token.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "FocalConfig.h"
#include "FocalDevice.h"
#include "FocalProtocol.h"
#include "FocalTrustlet.h"

namespace focaltech {

// Drives the Focaltech sensor and its QSEE trustlet.
class FocalHal {
  public:
    using Notify = std::function<void(const fingerprint_msg_t&)>;

    explicit FocalHal(Notify notify) : mNotify(std::move(notify)) {}
    ~FocalHal() { close(); }

    int open(const std::string& configPath);
    void close();

    int preEnroll(uint64_t* challenge);
    int enroll(const hw_auth_token_t& hat, uint32_t gid, uint32_t timeoutSec);
    int postEnroll();
    int getAuthenticatorId(uint64_t* id);
    int cancel();
    int enumerate();
    int remove(uint32_t gid, uint32_t fid);
    int setActiveGroup(uint32_t gid, const std::string& path);
    int authenticate(uint64_t operationId, uint32_t gid);

    const TaInfo& taInfo() const { return mTaInfo; }
    const std::string& driverVersion() const { return mDevice.driverVersion(); }

  private:
    using Clock = std::chrono::steady_clock;

    // State of the event currently being processed.
    struct EventContext {
        uint32_t frames = 0;
        uint32_t captures = 0;
        uint32_t code = 0;
        uint32_t scanCount = 0;
        int mode = MODE_IDLE;
        int stage = 0;
        EventMsg event = {};
    };

    // Settings of ff_config.json used by the HAL.
    struct Settings {
        int gesturingMode;
        int enrollType;
        int maxCaptureRetries;
        int authRescanTimingMs;
        int enrollBurst;
        bool hwResetBeforeScan;
        int keycodeSimulation;
        bool fastScreenUnlock;
        bool spiOnDemand;
        int healthCheckTiming;
        int healthCheckTimeoutMs;
    };

    void loadSettings();
    DriverConfig driverConfig() const;

    int initHal();
    int initDevice();
    int probeDevice();
    // Requires mMutex.
    int recoverDevice();

    void onDeviceEvent(DeviceEvent event);
    int onScreenChanged(bool on);

    void eventLoop();
    bool waitInterrupt();
    void postInterrupt();
    void clearInterrupt();
    int handleInterrupt();
    int handleFingerDown();
    int handleFingerUp();
    int handleTrustletEvent(EventContext& ctx, int status);

    void fingerTouched();
    void fingerReleased();
    int captureStage(EventContext& ctx);
    int processStage(EventContext& ctx);
    int errorStage(EventContext& ctx);
    int learningStage(EventContext& ctx);
    int updateTemplate(EventContext& ctx);
    int reportResult(EventContext& ctx);
    int reportEnrollment(const EventMsg& event);
    int reportAuthentication(const EventMsg& event);
    int reportKey(const EventMsg& event);
    bool isFilteredEnrollImage(const EventMsg& event) const;

    int ensureActiveGroup(uint32_t gid, const std::string& path);
    int selectGroup(uint32_t gid);
    static int32_t workModeFor(int mode) {
        switch (mode) {
            case MODE_ENROLL:
            case MODE_AUTH:
                return WORK_MODE_SCAN;
            case MODE_IDLE_LISTEN:
                return WORK_MODE_LISTEN;
            default:
                return WORK_MODE_IDLE;
        }
    }
    int idleMode() const { return mSettings.gesturingMode > 0 ? MODE_IDLE_LISTEN : MODE_IDLE; }
    int32_t remapAcquired(uint32_t status) const;

    void notify(const fingerprint_msg_t& msg);
    void notifyGuarded(const fingerprint_msg_t& msg);
    void notifyError(int32_t error);
    void notifyAcquired(int32_t info, bool guarded);

    void timerLoop();
    void armEnrollTimer(uint32_t timeoutMs);
    void restartEnrollTimer();
    void disarmEnrollTimer();
    void armHealthCheck(uint32_t delayMs);
    void disarmHealthCheck();
    void onHealthCheck();
    void armLazySave(uint32_t flags);
    void disarmLazySave();
    void onEnrollTimeout();
    void onLazySave();

    Notify mNotify;
    FocalConfig mConfig;
    Settings mSettings = {};
    FocalDevice mDevice;
    FocalTrustlet mTrustlet;
    TaInfo mTaInfo = {};

    std::atomic<bool> mRunning{false};
    std::atomic<bool> mCancelling{false};
    std::atomic<bool> mDeviceReady{false};
    std::atomic<bool> mTrustletFailed{false};
    std::atomic<bool> mGroupReady{false};
    std::atomic<bool> mLastAuthSucceeded{false};

    // Serializes event processing against cancel().
    std::mutex mEventMutex;
    // Protects the state below.
    std::mutex mMutex;
    int mMode = MODE_IDLE;
    bool mScreenOn = true;
    bool mFingerDown = false;
    bool mKeyPressed = false;
    bool mIdleWorkModeSet = false;
    bool mSuppressNotify = false;
    uint32_t mGid = 0;
    uint32_t mPreviousGid = 0;
    std::string mPath;
    Clock::time_point mEventStart;
    uint32_t mLazySaveFlags = 0;

    std::mutex mIrqMutex;
    std::condition_variable mIrqCv;
    bool mIrqPending = false;

    std::mutex mTimerMutex;
    std::condition_variable mTimerCv;
    std::chrono::milliseconds mEnrollTimeout{0};
    std::optional<Clock::time_point> mEnrollDeadline;
    std::optional<Clock::time_point> mLazyDeadline;
    std::optional<Clock::time_point> mHealthDeadline;

    std::thread mEventThread;
    std::thread mTimerThread;
};

}  // namespace focaltech
