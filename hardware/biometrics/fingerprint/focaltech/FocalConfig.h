/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <json/json.h>

#include <cstdint>
#include <string>

namespace focaltech {

// Reads ff_config.json. Keys are addressed as "section.name". Settings may
// be plain values or objects carrying the value in a "value" member.
class FocalConfig {
  public:
    bool load(const std::string& path);

    bool getBool(const std::string& key, bool def) const;
    int32_t getInt(const std::string& key, int32_t def) const;
    std::string getString(const std::string& key, const std::string& def) const;

    // The configuration as handed to the trustlet.
    const std::string& minified() const { return mMinified; }

  private:
    const Json::Value* find(const std::string& key) const;

    Json::Value mRoot;
    std::string mMinified;
};

}  // namespace focaltech
