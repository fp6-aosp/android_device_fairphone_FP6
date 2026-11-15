/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hardware/hw_auth_token.h>

#include <cstdint>

namespace focaltech {

// Trustlet commands.
enum : uint32_t {
    CMD_SEGMENTED = 0x1000,
    CMD_INIT = 0x1004,
    CMD_INIT_SPI = 0x1006,
    CMD_FREE_SPI = 0x1007,
    CMD_SET_SPI_SPEED = 0x1008,
    CMD_PROBE_DEVICE = 0x100a,
    CMD_INIT_DEVICE = 0x100b,
    CMD_ESD_HANDLE = 0x100c,
    CMD_SYNC_CONFIG = 0x100d,
    CMD_SYNC_STATS = 0x100e,
    CMD_SET_KM_KEY = 0x1011,
    CMD_CAPTURE_IMAGE = 0x1013,
    CMD_SAVE_DATA = 0x1014,
    CMD_UPDATE_TEMPLATE = 0x1015,
    CMD_CHECK_HEALTH = 0x1017,
    CMD_REPORT_EVENT = 0x1018,
    CMD_QUERY_EVENT_STATUS = 0x101d,
    CMD_QUERY_FINGER_STATUS = 0x101e,
    CMD_CONFIG_POWER_MODE = 0x101f,
    CMD_CONFIG_WORK_MODE = 0x1020,
    CMD_PRE_ENROLL = 0x2000,
    CMD_ENROLL = 0x2001,
    CMD_POST_ENROLL = 0x2002,
    CMD_GET_AUTHENTICATOR_ID = 0x2003,
    CMD_CANCEL = 0x2004,
    CMD_ENUMERATE = 0x2005,
    CMD_REMOVE = 0x2006,
    CMD_SET_ACTIVE_GROUP = 0x2007,
    CMD_AUTHENTICATE = 0x2008,
    CMD_INVALIDATE_AUTHENTICATOR_ID = 0x2009,
    CMD_RESET_LOCKOUT = 0x200a,
};

// Keymaster trustlet command that exports the HAT HMAC key.
constexpr uint32_t KM_CMD_GET_AUTH_TOKEN_KEY = 0x205;

// Error codes shared by the CA and the trustlet.
enum : int {
    FF_OK = 0,
    FF_ERR_GENERIC = -1,
    FF_ERR_NOENT = -2,
    FF_ERR_INTR = -4,
    FF_ERR_IO = -5,
    FF_ERR_AGAIN = -0xb,
    FF_ERR_NOMEM = -0xc,
    FF_ERR_TIMEOUT = -0x10,
    FF_ERR_BAD_PARAM = -200,
    FF_ERR_NULL = -0xc9,
    FF_ERR_OVERFLOW = -0xca,
    FF_ERR_PROTOCOL = -0xcb,
    FF_ERR_DEV_NOT_FOUND = -0xcd,
    FF_ERR_DEV_DEAD = -0xce,
    FF_ERR_LIMIT = -0xcf,
    FF_ERR_UNTRUSTED = -0xd0,
};

// Values of the trustlet's event status (CMD_QUERY_EVENT_STATUS).
enum : int {
    EVT_NONE = 0,
    EVT_DEAD_1 = 1,
    EVT_DEAD_2 = 2,
    EVT_DEAD_3 = 3,
    EVT_HEALTH = 4,
    EVT_FINGER_DOWN = 5,
    EVT_FINGER_UP = 6,
    EVT_RESERVED = 7,
    EVT_TA_8 = 8,
    EVT_TA_9 = 9,
    EVT_TA_10 = 10,
    EVT_DEAD_11 = 11,
};

// Operation modes of the HAL state machine.
enum : int {
    MODE_IDLE = 0,
    MODE_ENROLL = 1,
    MODE_AUTH = 2,
    MODE_IDLE_LISTEN = 3,
};

// Result kinds in EventMsg::kind.
enum : uint32_t {
    RESULT_NONE = 0,
    RESULT_ENROLL = 1,
    RESULT_AUTH = 2,
    RESULT_KEY = 3,
};

// Capture flags.
enum : uint32_t {
    CAP_FLAG_IMAGE = 0x1,
    CAP_FLAG_BASE = 0x2,
    CAP_FLAG_EXTRA = 0x40,
    CAP_FLAG_NO_BURST = 0xc0040000,
    EVT_FLAG_BASE = 0x8080000,
    EVT_FLAG_NO_ALGO_DATA = 0x4,
    EVT_FLAG_UPDATE_BASE = 0x80000,
};

// Flags of CMD_SAVE_DATA.
enum : uint32_t {
    SAVE_ENROLLED = 0x40000000,
    SAVE_TEMPLATE_UPDATE = 0x60000000,
};

// Status codes reported in EventMsg::status.
enum : uint32_t {
    ACQ_GOOD = 0,
    ACQ_NON_FINGERPRINT = 0x4b5,
};

constexpr uint32_t kCaptureDataSize = 0x20;
constexpr uint32_t kEventMsgSize = 0x2dc;
constexpr uint32_t kStatsSize = 0x230;
constexpr uint32_t kDeviceInfoSize = 0x3c;
constexpr uint32_t kTaInfoSize = 0xc0;

struct __attribute__((packed)) CaptureCtx {
    uint32_t hwReset;
    uint32_t unknown4;
    uint32_t unknown8;
    uint32_t scanCount;
    uint32_t one;
    uint32_t index;
    uint32_t flags;
    uint32_t dump;
};
static_assert(sizeof(CaptureCtx) == kCaptureDataSize);

// Exchanged with CMD_REPORT_EVENT / CMD_UPDATE_TEMPLATE. The trustlet fills
// in the result in place.
struct __attribute__((packed)) EventMsg {
    uint32_t nextMode;
    uint32_t event;
    uint32_t kind;
    uint32_t gid;
    uint32_t fid;
    uint32_t status;
    uint8_t reserved18[0xc];
    uint32_t remaining;
    hw_auth_token_t hat;
    uint8_t reserved6d[0x2bc - 0x6d];
    CaptureCtx capture;
};
static_assert(sizeof(EventMsg) == kEventMsgSize);
static_assert(offsetof(EventMsg, hat) == 0x28);
static_assert(offsetof(EventMsg, capture) == 0x2bc);

}  // namespace focaltech
