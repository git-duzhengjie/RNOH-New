/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <native_vsync/native_vsync.h>
#include <optional>
#include <string>

namespace rnoh {

class NativeVsyncHandle {
 public:
  explicit NativeVsyncHandle(std::string&& name) : m_name(std::move(name)) {
    m_nativeVSync = OH_NativeVSync_Create(m_name.data(), m_name.size());
  }

  NativeVsyncHandle(NativeVsyncHandle&& other) noexcept
      : m_name(std::move(other.m_name)), m_nativeVSync(other.m_nativeVSync) {
    other.m_nativeVSync = nullptr;
  }

  NativeVsyncHandle& operator=(NativeVsyncHandle&& other) noexcept {
    if (this != &other) {
      OH_NativeVSync_Destroy(m_nativeVSync);
      m_name = std::move(other.m_name);
      m_nativeVSync = other.m_nativeVSync;
      other.m_nativeVSync = nullptr;
    }
    return *this;
  }

  // no copy
  NativeVsyncHandle(const NativeVsyncHandle&) = delete;
  NativeVsyncHandle& operator=(const NativeVsyncHandle&) = delete;

  ~NativeVsyncHandle() {
    if (m_nativeVSync != nullptr) {
      OH_NativeVSync_Destroy(m_nativeVSync);
    }
  }

  int requestFrame(OH_NativeVSync_FrameCallback callback, void* data) {
    return OH_NativeVSync_RequestFrame(m_nativeVSync, callback, data);
  }

  std::optional<int64_t> getPeriodNs() const {
    if (m_nativeVSync == nullptr) {
      return std::nullopt;
    }
    long long periodNs = 0; // API expects long long*
    auto ret = OH_NativeVSync_GetPeriod(m_nativeVSync, &periodNs);
    if (ret == 0 && periodNs > 0) {
      return static_cast<int64_t>(periodNs);
    }
    return std::nullopt;
  }

  std::string m_name;
  OH_NativeVSync* m_nativeVSync = nullptr;
};

} // namespace rnoh
