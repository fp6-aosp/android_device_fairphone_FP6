/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "FocalHal"

#include "FocalHal.h"

#include <android-base/logging.h>
#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

namespace focaltech {

namespace {

constexpr char kDefaultDataPath[] = "/data/vendor_de/0/fpdata";
constexpr uint32_t kLazySaveDelayMs = 5000;
constexpr uint32_t kWakelockMs = 2000;
constexpr int kMaxStageSteps = 64;
constexpr int kMaxLearningFrames = 8;
constexpr uint32_t kMaxGroupId = 999;
constexpr uint32_t kVendorBase = 1000;
// Offset of the "template was updated" flag inside EventMsg.
constexpr size_t kTemplateUpdatedOffset = 0x2a4;
constexpr uint32_t kFilteredEnrollStatus = 0x3ea;
constexpr uint32_t kFakeFingerStatus = 0x4b4;
constexpr uint32_t kTooFastStatus = 5;
constexpr uint32_t kRemoteKeyNavUp = 4;
constexpr uint32_t kRemoteKeyNavDown = 8;
constexpr uint32_t kRemoteKeyIgnored = 0x55;
constexpr int kKeyPressMs = 10;
constexpr uint32_t kRemoteKeyDown = 0x80;
constexpr uint32_t kRemoteKeyUp = 0x81;

// Reasons why a capture stage ended.
enum : uint32_t {
    CODE_OK = 0,
    CODE_RETRIES_EXHAUSTED = 1,
    CODE_RESCAN_TIMEOUT = 2,
    CODE_FINGER_LEFT = 3,
    CODE_CAPTURE_FAILED = 4,
    CODE_CANCELLED = 5,
    CODE_FAKE_FINGER = 6,
};

enum Stage : int {
    STAGE_START = 0,
    STAGE_DONE = 1,
    STAGE_ERROR = 2,
    STAGE_CAPTURE = 3,
    STAGE_PROCESS = 4,
    STAGE_FINISH = 5,
};

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

bool makeDirectories(const std::string& path) {
    std::string current;
    size_t pos = 0;
    while (pos != std::string::npos) {
        pos = path.find('/', pos + 1);
        current = path.substr(0, pos);
        if (current.empty()) continue;
        if (mkdir(current.c_str(), 0770) != 0 && errno != EEXIST) {
            PLOG(ERROR) << "mkdir " << current;
            return false;
        }
    }
    return true;
}

fingerprint_msg_t makeError(int32_t error) {
    fingerprint_msg_t msg = {};
    msg.type = FINGERPRINT_ERROR;
    msg.data.error = static_cast<fingerprint_error_t>(error);
    return msg;
}

fingerprint_msg_t makeAcquired(int32_t info) {
    fingerprint_msg_t msg = {};
    msg.type = FINGERPRINT_ACQUIRED;
    msg.data.acquired.acquired_info = static_cast<fingerprint_acquired_info_t>(info);
    return msg;
}

fingerprint_msg_t makeEnrolling(uint32_t gid, uint32_t fid, uint32_t remaining) {
    fingerprint_msg_t msg = {};
    msg.type = FINGERPRINT_TEMPLATE_ENROLLING;
    msg.data.enroll.finger = {gid, fid};
    msg.data.enroll.samples_remaining = remaining;
    return msg;
}

fingerprint_msg_t makeIterator(fingerprint_msg_type_t type, uint32_t gid, uint32_t fid,
                               uint32_t remaining) {
    fingerprint_msg_t msg = {};
    msg.type = type;
    msg.data.removed.finger = {gid, fid};
    msg.data.removed.remaining_templates = remaining;
    return msg;
}

}  // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void FocalHal::loadSettings() {
    mSettings.gesturingMode = mConfig.getInt("device.gesturing_mode", 0);
    mSettings.enrollType = mConfig.getInt("device.enroll_type", 0);
    mSettings.maxCaptureRetries = mConfig.getInt("common.max_pre_processing_rescan_times", 0);
    mSettings.authRescanTimingMs = mConfig.getInt("common.max_authentication_rescan_timing", 1000);
    mSettings.enrollBurst =
            mSettings.enrollType == 0 ? 1 : mConfig.getInt("device.sliding_enroll_burst_num", 5);
    mSettings.hwResetBeforeScan = mConfig.getBool("device.enable_hw_reset_b4_scanning", false);
    mSettings.keycodeSimulation = mConfig.getInt("driver.keycode_simulation", 0);
    mSettings.fastScreenUnlock = mConfig.getBool("extension.enable_fast_screen_unlock", false);
    mSettings.spiOnDemand = mConfig.getBool("driver.spi_on_demand", false);
}

DriverConfig FocalHal::driverConfig() const {
    DriverConfig cfg = {};
    cfg.enableFasync = mConfig.getBool("driver.enable_fasync", false);
    cfg.gestureKeycode[0] = mConfig.getInt("driver.keycode_nav_left", 0x69);
    cfg.gestureKeycode[1] = mConfig.getInt("driver.keycode_nav_right", 0x6a);
    cfg.gestureKeycode[2] = mConfig.getInt("driver.keycode_nav_up", 0x67);
    cfg.gestureKeycode[3] = mConfig.getInt("driver.keycode_nav_down", 0x6c);
    cfg.gestureKeycode[4] = mConfig.getInt("driver.keycode_double_click", 0x1c);
    cfg.gestureKeycode[5] = mConfig.getInt("driver.keycode_click", 0x9e);
    cfg.gestureKeycode[6] = mConfig.getInt("driver.keycode_long_press", 0x74);
    cfg.gestureKeycode[7] = mConfig.getInt("driver.keycode_simulation", 0xac);
    cfg.enableSpidev = mConfig.getBool("driver.enable_spidev", false);
    cfg.spidevBus = mConfig.getInt("driver.spi_bus_num", 0);
    cfg.spidevCs = mConfig.getInt("driver.spi_c_s_num", 0);
    static constexpr const char* kGpioKeys[8] = {
            "driver.gpio_mosi_pin",  "driver.gpio_miso_pin",  "driver.gpio_ck_pin",
            "driver.gpio_cs_pin",    "driver.gpio_rst_pin",   "driver.gpio_int_pin",
            "driver.gpio_power_pin", "driver.gpio_iovcc_pin",
    };
    for (int i = 0; i < 8; i++) {
        cfg.gpio[i] = mConfig.getInt(kGpioKeys[i], -1);
    }
    cfg.logLevel = mConfig.getInt("diagnosis.framework_log_level", 2);
    cfg.logcatDriver = mConfig.getBool("diagnosis.enable_logcat_driver", true);
    return cfg;
}

int FocalHal::open(const std::string& configPath) {
    if (!mConfig.load(configPath)) return FF_ERR_GENERIC;
    loadSettings();
    mMode = idleMode();

    mRunning = true;
    int ret = mDevice.init(driverConfig(), [this](DeviceEvent event) { onDeviceEvent(event); });
    if (ret != 0) {
        mRunning = false;
        return ret;
    }

    mTimerThread = std::thread(&FocalHal::timerLoop, this);
    mEventThread = std::thread(&FocalHal::eventLoop, this);

    ret = initHal();
    if (ret != 0) {
        LOG(ERROR) << "initialization failed: " << ret;
        close();
        return ret;
    }

    LOG(INFO) << "driver " << mDevice.driverVersion() << ", trustlet " << mTaInfo.strings[0];
    return 0;
}

void FocalHal::close() {
    if (!mRunning.exchange(false)) return;

    {
        std::lock_guard<std::mutex> lock(mIrqMutex);
    }
    mIrqCv.notify_all();
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
    }
    mTimerCv.notify_all();

    if (mEventThread.joinable()) mEventThread.join();
    if (mTimerThread.joinable()) mTimerThread.join();

    mDeviceReady = false;
    mDevice.enableIrq(false);
    mTrustlet.freeSpi();
    mTrustlet.close();
    mDevice.free();
}

int FocalHal::initHal() {
    const std::string trustletPath =
            mConfig.getString("common.trustlet_path", "/vendor/firmware_mnt/image/focal64");
    const std::string keymasterPath =
            mConfig.getString("common.keymaster_path", "/vendor/firmware_mnt/image/keymaster64");

    FocalTrustlet::SpiClockFn spiClock;
    if (mSettings.spiOnDemand) {
        spiClock = [this](bool enable) { return mDevice.spiClock(enable); };
    }

    int ret = mTrustlet.open(trustletPath, keymasterPath, spiClock);
    LOG(INFO) << "trustlet open: " << ret;
    if (ret != 0) return ret;

    ret = mTrustlet.syncConfig(mConfig.minified());
    LOG(INFO) << "sync config: " << ret;
    if (ret != 0) {
        LOG(ERROR) << "failed to sync the configuration: " << ret;
        return ret;
    }

    if (!mSettings.spiOnDemand && !mDevice.spiClockEnabled()) {
        ret = mDevice.spiClock(true);
        if (ret != 0) return ret;
    }

    ret = mTrustlet.initSpi();
    if (ret != 0) {
        LOG(ERROR) << "failed to init the SPI bus: " << ret;
        return ret;
    }

    ret = probeDevice();
    if (ret != 0) return ret;

    return initDevice();
}

int FocalHal::probeDevice() {
    const bool rePower = mConfig.getBool("device.enable_re_power_b4_probing", false);
    DeviceInfo info;
    int ret;
    int attempts = 0;

    mDevice.enableIrq(false);
    mDevice.powerOn(true);
    do {
        if (rePower) mDevice.hwReset();
        ret = mTrustlet.probeDevice(rePower, &info);
        if (ret == FF_ERR_DEV_NOT_FOUND && attempts++ < 2) {
            mDevice.powerOn(false);
            sleepMs(100);
            mDevice.powerOn(true);
            sleepMs(10);
            if (mConfig.getBool("device.vio_is_1p8", true) && !rePower) {
                mTrustlet.configWorkMode(9);
            }
            ret = FF_ERR_AGAIN;
        }
    } while (ret == FF_ERR_AGAIN);

    if (ret != 0) {
        LOG(ERROR) << "failed to probe the sensor: " << ret;
        return ret;
    }
    LOG(INFO) << "sensor probed";
    clearInterrupt();
    return 0;
}

int FocalHal::initDevice() {
    DeviceInfo info;
    int ret = mTrustlet.initDevice(&info);
    LOG(INFO) << "init device: " << ret;
    if (ret != 0) {
        LOG(WARNING) << "failed to init the sensor: " << ret;
    } else {
        mDevice.setVendorInfo(mConfig.getString("driver.vendor_info", "focaltech").c_str());
    }
    clearInterrupt();
    mDevice.enableIrq(true);
    mDeviceReady = true;

    ret = mTrustlet.init(&mTaInfo);
    LOG(INFO) << "trustlet init: " << ret;
    if (ret != 0) {
        LOG(ERROR) << "failed to init the trustlet: " << ret;
        mTrustletFailed = true;
        return ret;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mScreenOn = true;
    mMode = idleMode();
    return 0;
}

int FocalHal::recoverDevice() {
    LOG(WARNING) << "sensor died, recovering";
    mDeviceReady = false;
    mDevice.enableIrq(false);

    int ret = FF_ERR_GENERIC;
    for (int attempt = 0; attempt < 8 && mRunning; attempt++) {
        mDevice.powerOn(false);
        mDevice.powerOn(true);
        mDevice.hwReset();
        sleepMs(5);

        ret = mTrustlet.esdHandle();
        if (ret != 0) continue;
        mTrustlet.configPowerMode(0);
        ret = mTrustlet.checkHealth(2);
        if (ret == 0) break;
    }

    if (ret != 0) {
        LOG(ERROR) << "failed to recover the sensor: " << ret;
        notifyError(FINGERPRINT_ERROR_HW_UNAVAILABLE);
        return ret;
    }

    mTrustlet.configWorkMode(workModeFor(mMode));
    clearInterrupt();
    mDevice.enableIrq(true);
    mDeviceReady = true;
    return 0;
}

// ---------------------------------------------------------------------------
// Device events
// ---------------------------------------------------------------------------

void FocalHal::onDeviceEvent(DeviceEvent event) {
    switch (event) {
        case DEV_EVENT_INTERRUPT:
            postInterrupt();
            break;
        case DEV_EVENT_SCREEN_ON:
            onScreenChanged(true);
            break;
        case DEV_EVENT_SCREEN_OFF:
            onScreenChanged(false);
            break;
    }
}

int FocalHal::onScreenChanged(bool on) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mScreenOn == on) return 0;
    mScreenOn = on;

    if (mTrustletFailed || !mDeviceReady) return FF_ERR_GENERIC;

    int ret = 0;
    if (!on) {
        int enrolled = mTrustlet.enumerate(nullptr);
        if (enrolled == 0) ret = mTrustlet.cancel();
        if ((mSettings.gesturingMode > 0 && enrolled == 0) || mMode == MODE_IDLE_LISTEN) {
            mIdleWorkModeSet = true;
        }
        if (mIdleWorkModeSet) ret = mTrustlet.configWorkMode(0);
    } else if (mIdleWorkModeSet) {
        mIdleWorkModeSet = false;
        ret = mTrustlet.configWorkMode(6);
    }
    return ret;
}

bool FocalHal::waitInterrupt() {
    std::unique_lock<std::mutex> lock(mIrqMutex);
    mIrqCv.wait(lock, [this] { return mIrqPending || !mRunning; });
    if (!mRunning) return false;
    mIrqPending = false;
    return true;
}

void FocalHal::postInterrupt() {
    {
        std::lock_guard<std::mutex> lock(mIrqMutex);
        mIrqPending = true;
    }
    mIrqCv.notify_one();
}

void FocalHal::clearInterrupt() {
    std::lock_guard<std::mutex> lock(mIrqMutex);
    mIrqPending = false;
}

void FocalHal::eventLoop() {
    while (mRunning) {
        if (!mDeviceReady || mTrustletFailed || !mGroupReady) {
            sleepMs(30);
            continue;
        }
        if (!waitInterrupt()) break;

        mDevice.acquireWakelock(kWakelockMs);
        handleInterrupt();
    }
}

int FocalHal::handleInterrupt() {
    std::lock_guard<std::mutex> eventLock(mEventMutex);
    EventContext ctx;
    int status;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mEventStart = Clock::now();
        status = mTrustlet.queryEventStatus();
    }
    if (status < 0) LOG(ERROR) << "failed to query the event status: " << status;
    LOG(DEBUG) << "event status " << status;

    int ret = 0;
    switch (status) {
        case EVT_DEAD_1:
        case EVT_DEAD_2:
        case EVT_DEAD_3:
        case EVT_DEAD_11: {
            std::lock_guard<std::mutex> lock(mMutex);
            ret = recoverDevice();
            break;
        }
        case EVT_FINGER_DOWN:
            ret = handleFingerDown();
            break;
        case EVT_FINGER_UP:
            ret = handleFingerUp();
            break;
        case EVT_TA_8:
        case EVT_TA_9:
        case EVT_TA_10:
            ret = handleTrustletEvent(ctx, status);
            break;
        default:
            break;
    }

    if (ret == FF_ERR_DEV_DEAD) {
        std::lock_guard<std::mutex> lock(mMutex);
        ret = recoverDevice();
    } else if (ret < 0) {
        notifyError(FINGERPRINT_ERROR_UNABLE_TO_PROCESS);
    }
    return ret;
}

int FocalHal::handleFingerUp() {
    int mode;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mode = mMode;
    }

    if (!mFingerDown) {
        mTrustlet.configWorkMode(workModeFor(mode));
        return 0;
    }

    mFingerDown = false;
    int32_t info = mConfig.getInt("notification.finger_left", 0);
    if (info > 0) notifyAcquired(info, false);

    EventMsg msg = {};
    msg.event = EVT_FINGER_UP;
    return mTrustlet.reportEvent(&msg);
}

int FocalHal::handleTrustletEvent(EventContext& ctx, int status) {
    EventMsg msg = {};
    msg.event = status;
    int ret = mTrustlet.reportEvent(&msg);
    ctx.event = msg;
    if (ret == 0) ret = reportResult(ctx);
    return ret;
}

int FocalHal::handleFingerDown() {
    EventContext ctx;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mMode != MODE_ENROLL && mMode != MODE_AUTH) {
            mTrustlet.configWorkMode(0);
            return 0;
        }
        ctx.mode = mMode;
    }

    int ret = 0;
    ctx.stage = STAGE_START;
    for (int step = 0; step < kMaxStageSteps; step++) {
        switch (ctx.stage) {
            case STAGE_START:
                fingerTouched();
                mFingerDown = true;
                ctx.stage = STAGE_CAPTURE;
                break;
            case STAGE_DONE:
                fingerReleased();
                return ret;
            case STAGE_ERROR:
                ret = errorStage(ctx);
                ctx.stage = STAGE_FINISH;
                break;
            case STAGE_CAPTURE:
                ret = captureStage(ctx);
                ctx.stage = ret == 0 ? STAGE_PROCESS : STAGE_ERROR;
                break;
            case STAGE_PROCESS:
                ret = processStage(ctx);
                if (ret == FF_ERR_AGAIN) {
                    ctx.stage = STAGE_CAPTURE;
                } else if (ret == FF_ERR_INTR) {
                    ctx.stage = STAGE_ERROR;
                } else {
                    if (ret == 0) ret = reportResult(ctx);
                    ctx.stage = STAGE_FINISH;
                }
                break;
            case STAGE_FINISH:
                if (ctx.mode == MODE_AUTH && mLastAuthSucceeded && ctx.code <= CODE_RESCAN_TIMEOUT) {
                    learningStage(ctx);
                }
                ctx.stage = STAGE_DONE;
                break;
        }
    }

    LOG(ERROR) << "event processing did not terminate";
    fingerReleased();
    return FF_ERR_GENERIC;
}

void FocalHal::fingerTouched() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mSettings.fastScreenUnlock && !mKeyPressed && !mScreenOn) {
            mKeyPressed = true;
            mDevice.reportKey(mSettings.keycodeSimulation, 1);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        if (mLazyDeadline) mLazyDeadline = Clock::now() + std::chrono::milliseconds(kLazySaveDelayMs);
    }
    mTimerCv.notify_all();

    int32_t info = mConfig.getInt("notification.finger_touched", 0);
    if (info > 0) notifyAcquired(info, false);
}

void FocalHal::fingerReleased() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSettings.fastScreenUnlock && mKeyPressed) {
        mKeyPressed = false;
        mDevice.reportKey(mSettings.keycodeSimulation, 0);
    }
}

// ---------------------------------------------------------------------------
// Capture and processing stages
// ---------------------------------------------------------------------------

int FocalHal::captureStage(EventContext& ctx) {
    uint32_t retries = 0;
    bool hwReset = false;
    int ret = 0;

    ctx.code = CODE_OK;
    while (true) {
        hwReset = mSettings.hwResetBeforeScan ? true : hwReset;
        bool done = true;

        std::lock_guard<std::mutex> lock(mMutex);
        if (ctx.mode == mMode && !mCancelling) {
            ctx.scanCount = ctx.stage == STAGE_FINISH ? 1
                            : mMode == MODE_ENROLL    ? mSettings.enrollBurst
                                                      : 1;

            CaptureCtx capture = {};
            capture.hwReset = hwReset;
            capture.scanCount = ctx.scanCount;
            capture.one = 1;
            capture.index = retries;
            if (mMode != MODE_ENROLL || mSettings.enrollType < 2) {
                capture.flags |= CAP_FLAG_NO_BURST;
            }
            if (ctx.stage == STAGE_FINISH) capture.flags |= CAP_FLAG_EXTRA;
            capture.flags |= CAP_FLAG_BASE;

            if (hwReset) {
                hwReset = false;
                mDevice.hwReset();
            }
            ret = mTrustlet.captureImage(&capture);
            ctx.scanCount = capture.scanCount;
            if (ret == 0 || ret == FF_ERR_AGAIN) ctx.captures++;

            if (ret == FF_ERR_AGAIN || ret == FF_ERR_TIMEOUT) {
                disarmLazySave();
                if (retries < static_cast<uint32_t>(mSettings.maxCaptureRetries) ||
                    (mSettings.enrollType > 1 && mMode == MODE_ENROLL)) {
                    if (ret == FF_ERR_TIMEOUT) hwReset = true;
                    retries++;
                    if (mTrustlet.queryFingerStatus() == 0) {
                        ctx.code = CODE_FINGER_LEFT;
                    } else {
                        done = false;
                    }
                } else {
                    ctx.code = CODE_RETRIES_EXHAUSTED;
                }
            } else if (ret == 0) {
                ctx.code = CODE_OK;
                int32_t info = mConfig.getInt("notification.image_captured", 0);
                if (ctx.stage != STAGE_FINISH && info >= static_cast<int32_t>(kVendorBase)) {
                    notifyAcquired(info, false);
                }
            } else {
                ctx.code = CODE_CAPTURE_FAILED;
            }
        } else {
            ctx.code = CODE_CANCELLED;
            ret = FF_ERR_INTR;
        }

        if (done) return ret;
    }
}

int FocalHal::processStage(EventContext& ctx) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (ctx.mode != mMode || mCancelling) {
        ctx.code = CODE_CANCELLED;
        return FF_ERR_INTR;
    }

    EventMsg msg = {};
    msg.event = ctx.frames == 0 ? 5 : 7;
    msg.capture.scanCount = ctx.scanCount;
    msg.capture.index = ctx.frames;
    msg.capture.flags = EVT_FLAG_BASE | EVT_FLAG_NO_ALGO_DATA;
    if (mMode == MODE_ENROLL && mSettings.enrollType > 1) msg.capture.flags |= CAP_FLAG_NO_BURST;

    int ret = mTrustlet.reportEvent(&msg);
    ctx.event = msg;
    ctx.frames++;
    if (ret != FF_ERR_AGAIN) return ret;

    disarmLazySave();
    if (msg.status == kFakeFingerStatus) {
        ctx.code = CODE_FAKE_FINGER;
        return 0;
    }

    if (mMode == MODE_AUTH) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - mEventStart);
        if (elapsed.count() >= mSettings.authRescanTimingMs) {
            ctx.code = CODE_RESCAN_TIMEOUT;
            return 0;
        }
    } else if (mSettings.enrollType < 2) {
        ctx.code = CODE_CAPTURE_FAILED;
        return 0;
    } else {
        restartEnrollTimer();
        if (msg.status == ACQ_GOOD) {
            notifyGuarded(makeEnrolling(msg.gid, msg.fid, msg.remaining));
        } else {
            notifyGuarded(makeAcquired(remapAcquired(msg.status)));
        }
    }
    return ret;
}

int FocalHal::errorStage(EventContext& ctx) {
    int32_t workMode;
    switch (ctx.code) {
        case CODE_RETRIES_EXHAUSTED:
        case CODE_CANCELLED:
            workMode = 0;
            break;
        case CODE_FINGER_LEFT:
            workMode = -2;
            break;
        case CODE_CAPTURE_FAILED:
            workMode = -1;
            break;
        default:
            return 0;
    }
    mTrustlet.configWorkMode(workMode);

    if (ctx.frames != 0) return reportResult(ctx);

    int32_t info = 0;
    if (ctx.code == CODE_FINGER_LEFT) {
        info = mConfig.getInt("notification.finger_leaving_too_fast", 0);
    } else if (ctx.code == CODE_RETRIES_EXHAUSTED) {
        info = mConfig.getInt("notification.acquired_imager_dirty", 0);
    }
    if (info != 0 && (ctx.mode == MODE_ENROLL || ctx.mode == MODE_AUTH)) {
        notifyAcquired(info, true);
    }
    return 0;
}

int FocalHal::learningStage(EventContext& ctx) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        ctx.mode = mMode;
    }
    ctx.frames = 0;

    for (int i = 0; i < kMaxLearningFrames; i++) {
        if (mTrustlet.queryFingerStatus() == 0) break;

        {
            std::lock_guard<std::mutex> lock(mTimerMutex);
            if (mLazyDeadline) mLazyDeadline = Clock::now() + std::chrono::milliseconds(kLazySaveDelayMs);
        }
        ctx.captures = 0;
        if (captureStage(ctx) != 0) break;
        ctx.frames++;
        if (updateTemplate(ctx) != 0) break;
    }
    mTrustlet.configWorkMode(2);
    return 0;
}

int FocalHal::updateTemplate(EventContext& ctx) {
    EventMsg msg = {};
    msg.capture.scanCount = ctx.event.capture.scanCount;
    msg.capture.index = ctx.frames - 1;
    msg.capture.flags = EVT_FLAG_UPDATE_BASE;
    if (ctx.stage == STAGE_FINISH) msg.capture.flags |= CAP_FLAG_EXTRA;

    int ret = mTrustlet.updateTemplate(&msg);
    if (ret == 0 && (msg.reserved6d[kTemplateUpdatedOffset - offsetof(EventMsg, reserved6d)] & 1)) {
        armLazySave(SAVE_TEMPLATE_UPDATE);
    }
    return ret;
}

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

int FocalHal::reportResult(EventContext& ctx) {
    const EventMsg& event = ctx.event;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mMode = event.nextMode;
    }

    const bool valid = (event.event >= 5 && event.event <= 10);
    if (!valid || mCancelling) return 0;

    switch (event.kind) {
        case RESULT_ENROLL:
            return reportEnrollment(event);
        case RESULT_AUTH: {
            int ret = reportAuthentication(event);
            updateTemplate(ctx);
            return ret;
        }
        case RESULT_KEY:
            return reportKey(event);
        default:
            return 0;
    }
}

bool FocalHal::isFilteredEnrollImage(const EventMsg& event) const {
    return event.status == kFilteredEnrollStatus &&
           mConfig.getBool("notification.enable_non_fingerprint_filter", false);
}

int FocalHal::reportEnrollment(const EventMsg& event) {
    const bool complete = event.remaining == 0;
    LOG(INFO) << "enroll progress remaining=" << event.remaining << " status=" << event.status;
    if (complete) {
        disarmEnrollTimer();
        int ret = mTrustlet.saveData(SAVE_ENROLLED);
        if (ret != 0) {
            LOG(ERROR) << "failed to save the template: " << ret;
            return ret;
        }
    } else {
        restartEnrollTimer();
    }

    if (isFilteredEnrollImage(event)) return 0;

    if (event.status == ACQ_GOOD) {
        notifyGuarded(makeEnrolling(event.gid, event.fid, event.remaining));
    } else {
        notifyGuarded(makeAcquired(remapAcquired(event.status)));
    }
    return 0;
}

int FocalHal::reportAuthentication(const EventMsg& event) {
    if (isFilteredEnrollImage(event)) return 0;

    const bool succeeded = event.fid != 0;
    LOG(INFO) << "auth result fid=" << event.fid << " status=" << event.status;
    notifyGuarded(makeAcquired(remapAcquired(event.status)));

    const bool filterNonFingerprint = mConfig.getBool("algorithm.enable_filter_non_fp_image", false);
    if (!filterNonFingerprint ||
        (event.status != ACQ_NON_FINGERPRINT && event.status != kTooFastStatus)) {
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_AUTHENTICATED;
        msg.data.authenticated.finger = {event.gid, event.fid};
        msg.data.authenticated.hat = event.hat;
        notifyGuarded(msg);

        if (succeeded) {
            int ret = mTrustlet.cancel();
            if (ret == 0) {
                std::lock_guard<std::mutex> lock(mMutex);
                mMode = idleMode();
            }
        }
    }

    mLastAuthSucceeded = succeeded;
    if (!succeeded) disarmLazySave();
    return 0;
}

int FocalHal::reportKey(const EventMsg& event) {
    int keycode = 0;
    switch (event.gid) {
        case kRemoteKeyNavUp:
            keycode = mConfig.getInt("driver.keycode_nav_up", 0);
            break;
        case kRemoteKeyNavDown:
            keycode = mConfig.getInt("driver.keycode_nav_down", 0);
            break;
        case kRemoteKeyDown:
            mDevice.reportKey(mConfig.getInt("driver.keycode_simulation", 0), 1);
            return 0;
        case kRemoteKeyUp:
            mDevice.reportKey(mConfig.getInt("driver.keycode_simulation", 0), 0);
            return 0;
        case kRemoteKeyIgnored:
            return 0;
        default:
            LOG(DEBUG) << "unhandled key event " << event.gid;
            return 0;
    }

    if (keycode > 0) {
        mDevice.reportKey(keycode, 1);
        sleepMs(kKeyPressMs);
        mDevice.reportKey(keycode, 0);
    }
    return 0;
}

int32_t FocalHal::remapAcquired(uint32_t status) const {
    auto get = [this](const char* name, int32_t def) {
        return mConfig.getInt(std::string("notification.") + name, def);
    };

    switch (status) {
        case 0:
            return get("acquired_good", 0);
        case 1:
            return get("acquired_partial", 1);
        case 2:
            return get("acquired_insufficient", 2);
        case 3:
            return get("acquired_imager_dirty", 3);
        case 4:
            return get("acquired_too_slow", 4);
        case 5:
            return get("acquired_too_fast", 5);
        case 0x4b1:
            return get("enrolled_finger", 0);
        case 0x4b2:
            return get("residual_fingerprint", 0);
        case 0x4b3:
            return get("foreign_body_residue", 0);
        case 0x4b4:
            return get("fake_fingerprint", 0);
        case 0x4b5:
            return get("non_fingerprint", 0);
        case 0x4b6:
            return get("rotate_too_less", 0);
        case 0x4b7:
            return get("rotate_too_much", 0);
        case 0x4b8:
            return get("finger_too_dry", 0);
        case 0x4b9:
            return get("finger_too_wet", 0);
        case 0x4ba:
            return get("finger_too_oily", 0);
        case 0x4bb:
        case 0x4bc:
            return status;
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// Notifications
// ---------------------------------------------------------------------------

void FocalHal::notify(const fingerprint_msg_t& msg) {
    if (mNotify) mNotify(msg);
}

void FocalHal::notifyGuarded(const fingerprint_msg_t& msg) {
    if (!mSuppressNotify) notify(msg);
}

void FocalHal::notifyError(int32_t error) {
    notify(makeError(error));
}

void FocalHal::notifyAcquired(int32_t info, bool guarded) {
    if (guarded) {
        notifyGuarded(makeAcquired(info));
    } else {
        notify(makeAcquired(info));
    }
}

// ---------------------------------------------------------------------------
// Timers
// ---------------------------------------------------------------------------

void FocalHal::timerLoop() {
    std::unique_lock<std::mutex> lock(mTimerMutex);
    while (mRunning) {
        std::optional<Clock::time_point> next;
        for (const auto& deadline : {mEnrollDeadline, mLazyDeadline}) {
            if (deadline && (!next || *deadline < *next)) next = deadline;
        }
        if (!next) {
            mTimerCv.wait(lock);
            continue;
        }
        if (mTimerCv.wait_until(lock, *next) != std::cv_status::timeout) continue;

        const auto now = Clock::now();
        const bool enrollFired = mEnrollDeadline && *mEnrollDeadline <= now;
        const bool lazyFired = mLazyDeadline && *mLazyDeadline <= now;
        if (enrollFired) mEnrollDeadline.reset();
        if (lazyFired) mLazyDeadline.reset();

        lock.unlock();
        if (enrollFired) onEnrollTimeout();
        if (lazyFired) onLazySave();
        lock.lock();
    }
}

void FocalHal::armEnrollTimer(uint32_t timeoutMs) {
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        mEnrollTimeout = std::chrono::milliseconds(timeoutMs);
        mEnrollDeadline = Clock::now() + mEnrollTimeout;
    }
    mTimerCv.notify_all();
}

void FocalHal::restartEnrollTimer() {
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        if (mEnrollDeadline) mEnrollDeadline = Clock::now() + mEnrollTimeout;
    }
    mTimerCv.notify_all();
}

void FocalHal::disarmEnrollTimer() {
    std::lock_guard<std::mutex> lock(mTimerMutex);
    mEnrollDeadline.reset();
}

void FocalHal::armLazySave(uint32_t flags) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mLazySaveFlags |= flags;
    }
    {
        std::lock_guard<std::mutex> lock(mTimerMutex);
        mLazyDeadline = Clock::now() + std::chrono::milliseconds(kLazySaveDelayMs);
    }
    mTimerCv.notify_all();
}

void FocalHal::disarmLazySave() {
    std::lock_guard<std::mutex> lock(mTimerMutex);
    mLazyDeadline.reset();
}

void FocalHal::onEnrollTimeout() {
    LOG(WARNING) << "enrollment timed out";
    notifyError(FINGERPRINT_ERROR_TIMEOUT);
    cancel();
}

void FocalHal::onLazySave() {
    uint32_t flags;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        flags = mLazySaveFlags;
        mLazySaveFlags = 0;
    }
    if (flags == 0) return;

    int ret = mTrustlet.saveData(flags);
    if (ret != 0) LOG(ERROR) << "failed to save the templates: " << ret;
}

// ---------------------------------------------------------------------------
// Operations
// ---------------------------------------------------------------------------

int FocalHal::ensureActiveGroup(uint32_t gid, const std::string& path) {
    mSuppressNotify = gid > kMaxGroupId;
    mPreviousGid = mGid;
    mGid = gid;

    if (!path.empty()) {
        mPath = path;
    } else if (mPath.empty()) {
        mPath = kDefaultDataPath;
    }
    makeDirectories(mPath);

    if (mTrustletFailed) return FF_ERR_GENERIC;

    int ret = mTrustlet.setActiveGroup(gid, mPath);
    LOG(INFO) << "active group " << gid << " " << mPath << ": " << ret;
    if (ret != 0) LOG(WARNING) << "failed to set the active group: " << ret;
    ret = mTrustlet.syncStatistics();
    mGroupReady = true;
    return ret;
}

int FocalHal::selectGroup(uint32_t gid) {
    if (gid == mGid) return 0;
    if (gid != mPreviousGid) return FF_ERR_GENERIC;
    const uint32_t previous = mPreviousGid;
    const std::string path = mPath;
    return ensureActiveGroup(previous, path);
}

int FocalHal::setActiveGroup(uint32_t gid, const std::string& path) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mGid == gid && mGroupReady) return 0;
    return ensureActiveGroup(gid, path);
}

int FocalHal::preEnroll(uint64_t* challenge) {
    std::lock_guard<std::mutex> lock(mMutex);
    *challenge = 0;
    return mTrustlet.preEnroll(challenge);
}

int FocalHal::postEnroll() {
    std::lock_guard<std::mutex> lock(mMutex);
    return mTrustlet.postEnroll();
}

int FocalHal::getAuthenticatorId(uint64_t* id) {
    std::lock_guard<std::mutex> lock(mMutex);
    *id = 0;
    return mTrustlet.getAuthenticatorId(id);
}

int FocalHal::enroll(const hw_auth_token_t& hat, uint32_t gid, uint32_t timeoutSec) {
    std::lock_guard<std::mutex> lock(mMutex);

    int ret = selectGroup(gid);
    if (ret != 0) return ret;
    if (mPath.empty() || mTrustletFailed) return FF_ERR_GENERIC;

    disarmLazySave();
    mLazySaveFlags = 0;

    uint32_t fid = gid;
    ret = mTrustlet.enroll(hat, &fid, false);
    LOG(INFO) << "enroll: " << ret;
    if (ret == FF_ERR_UNTRUSTED) {
        notifyError(mConfig.getInt("notification.untrusted_enrollment", 0));
        return 0;
    }
    if (ret == FF_ERR_LIMIT) {
        notifyError(FINGERPRINT_ERROR_NO_SPACE);
        return 0;
    }
    if (ret != 0) return ret;

    armEnrollTimer(timeoutSec * 1000);
    mMode = MODE_ENROLL;

    int32_t info = mConfig.getInt("notification.wait_for_enrollment", 0);
    if (info >= static_cast<int32_t>(kVendorBase)) notifyAcquired(info, false);
    return 0;
}

int FocalHal::authenticate(uint64_t operationId, uint32_t gid) {
    std::lock_guard<std::mutex> lock(mMutex);

    int ret = mPath.empty() && mDeviceReady ? ensureActiveGroup(gid, "") : selectGroup(gid);
    if (ret != 0) return ret;

    if (!mDeviceReady) {
        notifyError(FINGERPRINT_ERROR_HW_UNAVAILABLE);
        return 0;
    }

    if (mMode == MODE_ENROLL) disarmEnrollTimer();

    ret = mTrustlet.authenticate(operationId, gid);
    LOG(INFO) << "authenticate: " << ret;
    if (ret != 0) return ret;

    mMode = MODE_AUTH;
    mLastAuthSucceeded = false;

    int32_t info = mConfig.getInt("notification.wait_for_authentication", 0);
    if (info >= static_cast<int32_t>(kVendorBase)) notifyAcquired(info, false);
    return 0;
}

int FocalHal::cancel() {
    mCancelling = true;
    std::lock_guard<std::mutex> eventLock(mEventMutex);
    std::lock_guard<std::mutex> lock(mMutex);

    disarmEnrollTimer();
    int ret = mTrustlet.cancel();
    mMode = idleMode();

    mCancelling = false;
    return ret;
}

int FocalHal::enumerate() {
    std::lock_guard<std::mutex> lock(mMutex);

    std::vector<std::pair<uint32_t, uint32_t>> fingers;
    int count = mTrustlet.enumerate(&fingers);
    if (count <= 0) {
        notify(makeIterator(FINGERPRINT_TEMPLATE_ENUMERATING, 0, 0, 0));
        return count < 0 ? count : 0;
    }

    uint32_t remaining = fingers.size();
    for (const auto& [gid, fid] : fingers) {
        notify(makeIterator(FINGERPRINT_TEMPLATE_ENUMERATING, gid, fid, --remaining));
    }
    return 0;
}

int FocalHal::remove(uint32_t gid, uint32_t fid) {
    std::lock_guard<std::mutex> lock(mMutex);

    int ret = mPath.empty() ? ensureActiveGroup(gid, "") : selectGroup(gid);
    if (ret != 0) return ret;

    disarmLazySave();
    mLazySaveFlags = 0;

    std::vector<std::pair<uint32_t, uint32_t>> fingers;
    if (fid == 0) {
        int count = mTrustlet.enumerate(&fingers);
        if (count < 0) return count;
    } else {
        fingers.emplace_back(gid, fid);
    }

    uint32_t remaining = fingers.size();
    for (const auto& [fingerGid, fingerFid] : fingers) {
        int result = mTrustlet.remove(fingerGid, fingerFid);
        if (result == 0 || result == FF_ERR_NOENT) {
            notifyGuarded(makeIterator(FINGERPRINT_TEMPLATE_REMOVED, fingerGid, fingerFid,
                                       --remaining));
        } else {
            notifyGuarded(makeError(FINGERPRINT_ERROR_UNABLE_TO_REMOVE));
        }
    }

    if (fingers.empty()) {
        notifyGuarded(makeIterator(FINGERPRINT_TEMPLATE_REMOVED, gid, fid, 0));
    }
    return 0;
}

}  // namespace focaltech
