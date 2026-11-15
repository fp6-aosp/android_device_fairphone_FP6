/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "FocalDevice"

#include "FocalDevice.h"

#include <android-base/logging.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>

namespace focaltech {

namespace {

constexpr char kDevicePath[] = "/dev/focaltech_fp";
constexpr int kEventTypePoll = 3;

constexpr char kMagic = 'f';

struct Feature {
    uint16_t ver;
    uint16_t reserved1;
    uint32_t flags;
    uint32_t reserved2[32];
};

struct KeyEvent {
    uint32_t code;
    int32_t value;
};

constexpr unsigned long FF_IOC_INIT_DRIVER = _IO(kMagic, 0x00);
constexpr unsigned long FF_IOC_FREE_DRIVER = _IO(kMagic, 0x01);
constexpr unsigned long FF_IOC_RESET_DEVICE_HL = _IOW(kMagic, 0x02, uint32_t);
constexpr unsigned long FF_IOC_ENABLE_IRQ = _IO(kMagic, 0x03);
constexpr unsigned long FF_IOC_DISABLE_IRQ = _IO(kMagic, 0x04);
constexpr unsigned long FF_IOC_ENABLE_SPI_CLK = _IO(kMagic, 0x05);
constexpr unsigned long FF_IOC_DISABLE_SPI_CLK = _IO(kMagic, 0x06);
constexpr unsigned long FF_IOC_ENABLE_POWER = _IO(kMagic, 0x07);
constexpr unsigned long FF_IOC_DISABLE_POWER = _IO(kMagic, 0x08);
constexpr unsigned long FF_IOC_REPORT_KEY_EVENT = _IOW(kMagic, 0x09, KeyEvent);
constexpr unsigned long FF_IOC_SYNC_CONFIG = _IOWR(kMagic, 0x0a, DriverConfig);
constexpr unsigned long FF_IOC_GET_VERSION = _IOR(kMagic, 0x0b, char);
constexpr unsigned long FF_IOC_SET_VENDOR_INFO = _IOW(kMagic, 0x0c, char);
constexpr unsigned long FF_IOC_WAKELOCK_ACQUIRE = _IOW(kMagic, 0x0d, uint32_t);
constexpr unsigned long FF_IOC_GET_FEATURE = _IOR(kMagic, 0x11, Feature);
constexpr unsigned long FF_IOC_SET_EVENT_TYPE = _IOW(kMagic, 0x12, int);
constexpr unsigned long FF_IOC_GET_EVENT_INFO = _IOR(kMagic, 0x13, int);

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}  // namespace

FocalDevice::~FocalDevice() {
    free();
}

int FocalDevice::init(const DriverConfig& config, EventCallback callback) {
    mCallback = std::move(callback);

    mFd = open(kDevicePath, O_RDWR | O_CLOEXEC);
    if (mFd < 0) {
        PLOG(ERROR) << "open " << kDevicePath;
        return -EIO;
    }
    if (pipe2(mPipe, O_CLOEXEC) != 0) {
        PLOG(ERROR) << "pipe2";
        free();
        return -EIO;
    }

    DriverConfig cfg = config;
    if (ioctl(mFd, FF_IOC_SYNC_CONFIG, &cfg) != 0) {
        PLOG(ERROR) << "FF_IOC_SYNC_CONFIG";
        free();
        return -EIO;
    }
    if (ioctl(mFd, FF_IOC_INIT_DRIVER) != 0) {
        PLOG(ERROR) << "FF_IOC_INIT_DRIVER";
        free();
        return -EIO;
    }

    Feature feature = {};
    if (ioctl(mFd, FF_IOC_GET_FEATURE, &feature) != 0) {
        PLOG(WARNING) << "FF_IOC_GET_FEATURE";
    } else if (feature.ver != 0) {
        int eventType = kEventTypePoll;
        if (ioctl(mFd, FF_IOC_SET_EVENT_TYPE, &eventType) != 0) {
            PLOG(ERROR) << "FF_IOC_SET_EVENT_TYPE";
            free();
            return -EIO;
        }
    }

    char version[32] = {};
    if (ioctl(mFd, FF_IOC_GET_VERSION, version) == 0) {
        version[sizeof(version) - 1] = '\0';
        mDriverVersion = version;
    } else {
        mDriverVersion = "unknown";
    }

    mThread = std::thread(&FocalDevice::pollLoop, this);
    return 0;
}

void FocalDevice::free() {
    if (mThread.joinable()) {
        char c = 0;
        if (write(mPipe[1], &c, 1) != 1) PLOG(ERROR) << "write";
        mThread.join();
    }
    if (mFd >= 0) {
        ioctl(mFd, FF_IOC_FREE_DRIVER);
        close(mFd);
        mFd = -1;
    }
    for (int& fd : mPipe) {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }
    mIrqEnabled = false;
    mSpiClockEnabled = false;
}

void FocalDevice::pollLoop() {
    pollfd fds[2] = {
            {.fd = mFd, .events = POLLIN | POLLRDNORM, .revents = 0},
            {.fd = mPipe[0], .events = POLLIN, .revents = 0},
    };

    while (true) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            if (errno != EINTR) {
                PLOG(ERROR) << "poll";
                sleepMs(10);
            }
            continue;
        }
        if (fds[1].revents & POLLIN) {
            return;
        }
        if (!(fds[0].revents & (POLLIN | POLLRDNORM))) {
            continue;
        }

        int bits = 0;
        if (ioctl(mFd, FF_IOC_GET_EVENT_INFO, &bits) < 0) {
            PLOG(ERROR) << "FF_IOC_GET_EVENT_INFO";
            sleepMs(10);
            continue;
        }
        for (int event = DEV_EVENT_INTERRUPT; event <= DEV_EVENT_SCREEN_OFF; event++) {
            if (!(bits & (1 << event))) continue;
            if (event == DEV_EVENT_INTERRUPT && !mIrqEnabled) continue;
            mCallback(static_cast<DeviceEvent>(event));
        }
    }
}

int FocalDevice::enableIrq(bool enable) {
    if (ioctl(mFd, enable ? FF_IOC_ENABLE_IRQ : FF_IOC_DISABLE_IRQ) != 0) {
        PLOG(ERROR) << (enable ? "FF_IOC_ENABLE_IRQ" : "FF_IOC_DISABLE_IRQ");
        return -EIO;
    }
    mIrqEnabled = enable;
    return 0;
}

int FocalDevice::spiClock(bool enable) {
    if (ioctl(mFd, enable ? FF_IOC_ENABLE_SPI_CLK : FF_IOC_DISABLE_SPI_CLK) != 0) {
        PLOG(ERROR) << (enable ? "FF_IOC_ENABLE_SPI_CLK" : "FF_IOC_DISABLE_SPI_CLK");
        return -EIO;
    }
    mSpiClockEnabled = enable;
    return 0;
}

int FocalDevice::setResetPin(bool level) {
    if (ioctl(mFd, FF_IOC_RESET_DEVICE_HL, static_cast<unsigned long>(level)) != 0) {
        PLOG(ERROR) << "FF_IOC_RESET_DEVICE_HL";
        return -EIO;
    }
    return 0;
}

int FocalDevice::powerOn(bool enable) {
    if (enable) {
        setResetPin(false);
        sleepMs(5);
        if (ioctl(mFd, FF_IOC_ENABLE_POWER) != 0) PLOG(ERROR) << "FF_IOC_ENABLE_POWER";
        sleepMs(10);
        setResetPin(true);
        sleepMs(10);
    } else {
        setResetPin(false);
        sleepMs(5);
        if (ioctl(mFd, FF_IOC_DISABLE_POWER) != 0) PLOG(ERROR) << "FF_IOC_DISABLE_POWER";
        sleepMs(5);
    }
    return 0;
}

int FocalDevice::hwReset() {
    setResetPin(false);
    sleepMs(5);
    setResetPin(true);
    sleepMs(10);
    return 0;
}

int FocalDevice::reportKey(uint32_t code, int32_t value) {
    KeyEvent event = {code, value};
    if (ioctl(mFd, FF_IOC_REPORT_KEY_EVENT, &event) != 0) {
        PLOG(ERROR) << "FF_IOC_REPORT_KEY_EVENT";
        return -EIO;
    }
    return 0;
}

void FocalDevice::setVendorInfo(const char* info) {
    ioctl(mFd, FF_IOC_SET_VENDOR_INFO, info);
}

void FocalDevice::acquireWakelock(uint32_t timeoutMs) {
    ioctl(mFd, FF_IOC_WAKELOCK_ACQUIRE, &timeoutMs);
}

}  // namespace focaltech
