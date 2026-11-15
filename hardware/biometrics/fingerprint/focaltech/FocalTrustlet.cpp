/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "FocalTrustlet"

#include "FocalTrustlet.h"

#include <android-base/logging.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

extern "C" {
struct QSEECom_handle {
    void* ion_sbuffer;
};

int QSEECom_start_app(QSEECom_handle** handle, const char* path, const char* appname,
                      uint32_t sbSize);
int QSEECom_shutdown_app(QSEECom_handle** handle);
int QSEECom_send_cmd(QSEECom_handle* handle, void* sendBuf, uint32_t sendLen, void* respBuf,
                     uint32_t respLen);
}

namespace focaltech {

namespace {

constexpr uint32_t kSendSize = 0x78000;
constexpr uint32_t kRespSize = 0x8040;
constexpr uint32_t kBufferSize = kSendSize + kRespSize;
constexpr uint32_t kHeaderSize = 0x10;
constexpr uint32_t kResponseFlag = 0x80000000;

constexpr uint32_t kKeymasterBufferSize = 0x840;
constexpr uint32_t kKeymasterRequestSize = 0x40;
constexpr uint32_t kKeymasterResponseSize = 0x800;

struct Header {
    uint32_t cmd;
    uint32_t len;
    uint32_t err;
    uint32_t time;
};
static_assert(sizeof(Header) == kHeaderSize);

bool splitPath(const std::string& path, std::string* dir, std::string* name) {
    size_t pos = path.rfind('/');
    if (pos == std::string::npos || pos + 1 >= path.size()) return false;
    *dir = path.substr(0, pos);
    *name = path.substr(pos + 1);
    return true;
}

template <typename T>
int callValue(int ret, uint32_t outLen, const T& value, T* out) {
    if (ret == 0) {
        if (outLen < sizeof(T)) return FF_ERR_PROTOCOL;
        *out = value;
    }
    return ret;
}

}  // namespace

int FocalTrustlet::open(const std::string& trustletPath, const std::string& keymasterPath,
                        SpiClockFn spiClock) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mHandle) return 0;

    std::string dir, name;
    if (!splitPath(trustletPath, &dir, &name)) {
        LOG(ERROR) << "invalid trustlet path: " << trustletPath;
        return FF_ERR_GENERIC;
    }

    LOG(INFO) << "attaching " << trustletPath;
    if (QSEECom_start_app(&mHandle, dir.c_str(), name.c_str(), kBufferSize) != 0) {
        PLOG(ERROR) << "QSEECom_start_app " << trustletPath;
        mHandle = nullptr;
        return FF_ERR_GENERIC;
    }
    mBuffer = static_cast<uint8_t*>(mHandle->ion_sbuffer);
    memset(mBuffer, 0, kBufferSize);
    mSpiClock = std::move(spiClock);

    int ret = provisionKeymasterKey(keymasterPath);
    if (ret != 0) {
        LOG(ERROR) << "failed to provision the authentication token key: " << ret;
    }
    return 0;
}

void FocalTrustlet::close() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mHandle) return;
    if (QSEECom_shutdown_app(&mHandle) != 0) {
        PLOG(ERROR) << "QSEECom_shutdown_app";
    }
    mHandle = nullptr;
    mBuffer = nullptr;
}

int FocalTrustlet::provisionKeymasterKey(const std::string& keymasterPath) {
    std::string dir, name;
    if (!splitPath(keymasterPath, &dir, &name)) return FF_ERR_GENERIC;

    QSEECom_handle* km = nullptr;
    if (QSEECom_start_app(&km, dir.c_str(), name.c_str(), kKeymasterBufferSize) != 0) {
        PLOG(ERROR) << "QSEECom_start_app " << keymasterPath;
        return FF_ERR_GENERIC;
    }

    int ret = FF_ERR_GENERIC;
    uint8_t* buf = static_cast<uint8_t*>(km->ion_sbuffer);
    memset(buf, 0, kKeymasterRequestSize);
    uint32_t* req = reinterpret_cast<uint32_t*>(buf);
    req[0] = KM_CMD_GET_AUTH_TOKEN_KEY;
    req[1] = 2;

    uint8_t* resp = buf + kKeymasterRequestSize;
    if (QSEECom_send_cmd(km, buf, kKeymasterRequestSize, resp, kKeymasterResponseSize) != 0) {
        PLOG(ERROR) << "QSEECom_send_cmd keymaster";
    } else {
        const uint32_t* r = reinterpret_cast<const uint32_t*>(resp);
        uint32_t offset = r[1];
        uint32_t size = r[2];
        if (r[0] != 0) {
            LOG(ERROR) << "keymaster returned status " << r[0];
        } else if (offset < 3 * sizeof(uint32_t) || offset > kKeymasterResponseSize ||
                   size > kKeymasterResponseSize - offset) {
            LOG(ERROR) << "keymaster response out of bounds: offset " << offset << ", size "
                       << size;
        } else {
            ret = callLocked(CMD_SET_KM_KEY, resp + offset, size, nullptr, 0, nullptr);
        }
    }

    if (QSEECom_shutdown_app(&km) != 0) {
        PLOG(ERROR) << "QSEECom_shutdown_app keymaster";
    }
    return ret;
}

int FocalTrustlet::call(uint32_t cmd, const void* in, uint32_t inLen, void* out, uint32_t outCap,
                        uint32_t* outLen) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mHandle) return FF_ERR_GENERIC;

    if (mSpiClock) mSpiClock(true);
    int ret = callLocked(cmd, in, inLen, out, outCap, outLen);
    if (mSpiClock) mSpiClock(false);
    return ret;
}

int FocalTrustlet::callLocked(uint32_t cmd, const void* in, uint32_t inLen, void* out,
                              uint32_t outCap, uint32_t* outLen) {
    if (inLen > kSendSize - kHeaderSize) return FF_ERR_OVERFLOW;
    if (inLen != 0 && in == nullptr) return FF_ERR_NULL;

    Header* header = reinterpret_cast<Header*>(mBuffer);
    uint8_t* data = mBuffer + kHeaderSize;
    header->cmd = cmd;
    header->len = inLen;
    header->err = 0;
    header->time = 0;
    if (inLen != 0) memmove(data, in, inLen);

    if (QSEECom_send_cmd(mHandle, mBuffer, kSendSize, mBuffer + kSendSize, kRespSize) != 0) {
        PLOG(ERROR) << "QSEECom_send_cmd cmd=0x" << std::hex << cmd;
        return FF_ERR_GENERIC;
    }
    printTrustletLog();
    if (!(header->cmd & kResponseFlag)) {
        LOG(ERROR) << "bad response to cmd 0x" << std::hex << cmd;
        return FF_ERR_PROTOCOL;
    }
    if (header->len > kSendSize - kHeaderSize) return FF_ERR_PROTOCOL;

    if (outLen) *outLen = header->len;
    if (out && outCap != 0) {
        memcpy(out, data, std::min(outCap, header->len));
    }
    return static_cast<int>(header->err);
}

// The trustlet appends its log as entries of '!', level, a 32 byte tag and the text.
void FocalTrustlet::printTrustletLog() {
    constexpr uint8_t kPending = '*';
    constexpr uint8_t kEntry = '!';
    constexpr uint8_t kConsumed = 0xde;
    constexpr size_t kTextOffset = 0x25;

    uint8_t* log = mBuffer + kSendSize;
    uint8_t* end = log + kRespSize;
    if (log[0] != kPending) return;

    uint8_t* entry = log;
    *entry = kEntry;
    while (entry < end && *entry == kEntry) {
        int32_t level;
        memcpy(&level, entry + 1, sizeof(level));
        if (level > 6 || entry + kTextOffset >= end) break;

        const char* tag = reinterpret_cast<const char*>(entry + 6);
        const char* text = reinterpret_cast<const char*>(entry + kTextOffset);
        size_t len = strnlen(text, end - entry - kTextOffset);
        if (entry + kTextOffset + len >= end) break;

        android::base::LogSeverity severity;
        switch (level) {
            case 1:
                severity = android::base::VERBOSE;
                break;
            case 2:
                severity = android::base::DEBUG;
                break;
            case 3:
                severity = android::base::DEBUG;
                break;
            case 4:
                severity = android::base::WARNING;
                break;
            default:
                severity = android::base::ERROR;
                break;
        }
        LOG(severity) << "TA " << std::string(tag, strnlen(tag, 31)) << ": " << text;
        entry += kTextOffset + len + 1;
    }
    log[0] = kConsumed;
}

int FocalTrustlet::init(TaInfo* info) {
    uint32_t len = 0;
    int ret = call(CMD_INIT, nullptr, 0, info, sizeof(*info), &len);
    if (ret == 0 && len < sizeof(*info)) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::syncConfig(const std::string& json) {
    return call(CMD_SYNC_CONFIG, json.c_str(), json.size() + 1);
}

int FocalTrustlet::syncStatistics() {
    uint8_t stats[kStatsSize] = {};
    return call(CMD_SYNC_STATS, stats, sizeof(stats));
}

int FocalTrustlet::initSpi() {
    return call(CMD_INIT_SPI, nullptr, 0);
}

int FocalTrustlet::freeSpi() {
    return call(CMD_FREE_SPI, nullptr, 0);
}

int FocalTrustlet::probeDevice(bool hwReset, DeviceInfo* info) {
    uint8_t flag = hwReset ? 1 : 0;
    uint32_t len = 0;
    int ret = call(CMD_PROBE_DEVICE, &flag, 1, info->raw, sizeof(info->raw), &len);
    if (ret == 0 && len < sizeof(info->raw)) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::initDevice(DeviceInfo* info) {
    uint32_t len = 0;
    int ret = call(CMD_INIT_DEVICE, nullptr, 0, info->raw, sizeof(info->raw), &len);
    if (ret == 0 && len < sizeof(info->raw)) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::esdHandle() {
    return call(CMD_ESD_HANDLE, nullptr, 0);
}

int FocalTrustlet::checkHealth(int32_t mode) {
    return call(CMD_CHECK_HEALTH, &mode, sizeof(mode));
}

int FocalTrustlet::configPowerMode(int32_t mode) {
    return call(CMD_CONFIG_POWER_MODE, &mode, sizeof(mode));
}

int FocalTrustlet::configWorkMode(int32_t mode) {
    return call(CMD_CONFIG_WORK_MODE, &mode, sizeof(mode));
}

int FocalTrustlet::queryEventStatus() {
    uint32_t arg = 0;
    return call(CMD_QUERY_EVENT_STATUS, &arg, sizeof(arg));
}

int FocalTrustlet::queryFingerStatus() {
    uint32_t arg = 1;
    return call(CMD_QUERY_FINGER_STATUS, &arg, sizeof(arg));
}

int FocalTrustlet::captureImage(CaptureCtx* capture) {
    if (capture->flags & CAP_FLAG_IMAGE) return FF_ERR_BAD_PARAM;
    uint32_t len = 0;
    int ret = call(CMD_CAPTURE_IMAGE, capture, sizeof(*capture), capture, sizeof(*capture), &len);
    if (len < sizeof(*capture) && (ret == 0 || ret == FF_ERR_AGAIN)) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::reportEvent(EventMsg* event) {
    uint32_t len = 0;
    int ret = call(CMD_REPORT_EVENT, event, sizeof(*event), event, sizeof(*event), &len);
    if (len < sizeof(*event) && (ret == 0 || ret == FF_ERR_AGAIN)) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::updateTemplate(EventMsg* event) {
    uint32_t len = 0;
    int ret = call(CMD_UPDATE_TEMPLATE, event, sizeof(*event), event, sizeof(*event), &len);
    if (len < sizeof(*event) && ret == 0) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::saveData(uint32_t flags) {
    uint8_t stats[kStatsSize];
    uint32_t len = 0;
    int ret = call(CMD_SAVE_DATA, &flags, sizeof(flags), stats, sizeof(stats), &len);
    if (ret == 0 && len < sizeof(stats)) return FF_ERR_PROTOCOL;
    return ret;
}

int FocalTrustlet::preEnroll(uint64_t* challenge) {
    uint64_t value = 0;
    uint32_t len = 0;
    int ret = call(CMD_PRE_ENROLL, nullptr, 0, &value, sizeof(value), &len);
    return callValue(ret, len, value, challenge);
}

int FocalTrustlet::enroll(const hw_auth_token_t& hat, uint32_t* gid, bool flag) {
    constexpr uint32_t kSize = sizeof(hw_auth_token_t) + sizeof(uint32_t) + 1;
    uint8_t request[kSize];
    memcpy(request, &hat, sizeof(hat));
    memcpy(request + sizeof(hat), gid, sizeof(*gid));
    request[kSize - 1] = flag ? 1 : 0;

    uint32_t value = *gid;
    uint32_t len = 0;
    int ret = call(CMD_ENROLL, request, kSize, &value, sizeof(value), &len);
    if (ret == 0 && len >= sizeof(value)) *gid = value;
    return ret;
}

int FocalTrustlet::postEnroll() {
    return call(CMD_POST_ENROLL, nullptr, 0);
}

int FocalTrustlet::getAuthenticatorId(uint64_t* id) {
    uint64_t value = 0;
    uint32_t len = 0;
    int ret = call(CMD_GET_AUTHENTICATOR_ID, nullptr, 0, &value, sizeof(value), &len);
    return callValue(ret, len, value, id);
}

int FocalTrustlet::invalidateAuthenticatorId(uint64_t* id) {
    uint64_t value = 0;
    uint32_t len = 0;
    int ret = call(CMD_INVALIDATE_AUTHENTICATOR_ID, nullptr, 0, &value, sizeof(value), &len);
    return callValue(ret, len, value, id);
}

int FocalTrustlet::cancel() {
    return call(CMD_CANCEL, nullptr, 0);
}

int FocalTrustlet::enumerate(std::vector<std::pair<uint32_t, uint32_t>>* fingers) {
    constexpr uint32_t kCapacity = 10;
    uint32_t capacity = fingers ? kCapacity : 0;
    uint8_t response[sizeof(uint32_t) + kCapacity * 2 * sizeof(uint32_t)];
    uint32_t len = 0;

    int count = call(CMD_ENUMERATE, &capacity, sizeof(capacity), response, sizeof(response), &len);
    if (count < 0 || !fingers) return count;

    if (static_cast<uint32_t>(count) > kCapacity) return FF_ERR_NOMEM;
    if (len < sizeof(uint32_t) + count * 2 * sizeof(uint32_t)) return FF_ERR_PROTOCOL;

    fingers->clear();
    const uint32_t* entries = reinterpret_cast<const uint32_t*>(response + sizeof(uint32_t));
    for (int i = 0; i < count; i++) {
        fingers->emplace_back(entries[2 * i], entries[2 * i + 1]);
    }
    return count;
}

int FocalTrustlet::remove(uint32_t gid, uint32_t fid) {
    uint32_t request[2] = {gid, fid};
    return call(CMD_REMOVE, request, sizeof(request));
}

int FocalTrustlet::setActiveGroup(uint32_t gid, const std::string& path) {
    std::vector<uint8_t> request(sizeof(gid) + path.size() + 1);
    memcpy(request.data(), &gid, sizeof(gid));
    memcpy(request.data() + sizeof(gid), path.c_str(), path.size() + 1);
    return call(CMD_SET_ACTIVE_GROUP, request.data(), request.size());
}

int FocalTrustlet::authenticate(uint64_t operationId, uint32_t gid) {
    uint8_t request[14] = {};
    memcpy(request, &operationId, sizeof(operationId));
    memcpy(request + 8, &gid, sizeof(gid));
    return call(CMD_AUTHENTICATE, request, sizeof(request));
}

int FocalTrustlet::resetLockout(const hw_auth_token_t& hat) {
    return call(CMD_RESET_LOCKOUT, &hat, sizeof(hat));
}

}  // namespace focaltech
