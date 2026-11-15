/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "FocalConfig"

#include "FocalConfig.h"

#include <android-base/file.h>
#include <android-base/logging.h>

#include <cctype>
#include <memory>

namespace focaltech {

namespace {

std::string minify(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    bool inString = false;
    bool escaped = false;
    for (char c : text) {
        if (inString) {
            out.push_back(c);
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
        } else if (c == '"') {
            inString = true;
            out.push_back(c);
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            out.push_back(c);
        }
    }
    return out;
}

}  // namespace

bool FocalConfig::load(const std::string& path) {
    std::string text;
    if (!android::base::ReadFileToString(path, &text)) {
        PLOG(ERROR) << "failed to read " << path;
        return false;
    }

    Json::CharReaderBuilder builder;
    std::string errors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(text.data(), text.data() + text.size(), &mRoot, &errors)) {
        LOG(ERROR) << "failed to parse " << path << ": " << errors;
        return false;
    }

    mMinified = minify(text);
    return true;
}

const Json::Value* FocalConfig::find(const std::string& key) const {
    size_t dot = key.find('.');
    if (dot == std::string::npos) return nullptr;

    const Json::Value& section = mRoot[key.substr(0, dot)];
    if (!section.isObject()) return nullptr;

    const Json::Value& node = section[key.substr(dot + 1)];
    if (node.isNull()) return nullptr;
    if (node.isObject()) {
        return node.isMember("value") ? &node["value"] : nullptr;
    }
    return &node;
}

bool FocalConfig::getBool(const std::string& key, bool def) const {
    const Json::Value* v = find(key);
    if (!v) return def;
    if (v->isBool()) return v->asBool();
    if (v->isIntegral()) return v->asInt() != 0;
    return def;
}

int32_t FocalConfig::getInt(const std::string& key, int32_t def) const {
    const Json::Value* v = find(key);
    if (!v) return def;
    if (v->isInt()) return v->asInt();
    if (v->isBool()) return v->asBool() ? 1 : 0;
    return def;
}

std::string FocalConfig::getString(const std::string& key, const std::string& def) const {
    const Json::Value* v = find(key);
    return v && v->isString() ? v->asString() : def;
}

}  // namespace focaltech
