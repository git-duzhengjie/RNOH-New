/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "FreezeStackCollector.h"

#include <chrono>
#include <string>
#include <string_view>

namespace rnoh {

namespace {

void appendStringView(std::string& output, std::string_view value) {
  output.append(value.data(), value.size());
}

} // namespace

FreezeStackCollector& FreezeStackCollector::getInstance() {
  static FreezeStackCollector instance;
  return instance;
}

void FreezeStackCollector::registerRuntime(RuntimeInfo info) {
  auto shouldClearCache = false;
  {
    auto lock = std::lock_guard(m_runtimeMutex);
    if (info.hermesRuntime == nullptr) {
      shouldClearCache = m_runtimeInfos.erase(info.rnInstanceId) > 0;
    } else {
      m_runtimeInfos[info.rnInstanceId] = info;
      shouldClearCache = true;
    }
  }

  if (shouldClearCache) {
    clearCache(info.rnInstanceId);
  }
}

void FreezeStackCollector::unregisterRuntime(size_t rnInstanceId) {
  auto shouldClearCache = false;
  {
    auto lock = std::lock_guard(m_runtimeMutex);
    auto it = m_runtimeInfos.find(rnInstanceId);
    if (it == m_runtimeInfos.end()) {
      return;
    }

#if USE_HERMES
    if (it->second.hermesRuntime != nullptr) {
      try {
        auto* iHermes =
            static_cast<facebook::hermes::IHermes*>(it->second.hermesRuntime);
        iHermes->unregisterForProfiling();
      } catch (...) {
      }
    }
#endif

    m_runtimeInfos.erase(it);
    shouldClearCache = true;
  }

  if (shouldClearCache) {
    clearCache(rnInstanceId);
  }
}

void FreezeStackCollector::cacheStackResult(
    size_t rnInstanceId,
    const std::string& stack) {
  if (stack.empty()) {
    return;
  }

  auto lock = std::lock_guard(m_cacheMutex);
  auto& cacheEntry = m_cacheByInstance[rnInstanceId];
  cacheEntry.stack = stack;
  cacheEntry.capturedAt = std::chrono::steady_clock::now();
  cacheEntry.hasValue = true;
}

FreezeStackCollector::CacheResult FreezeStackCollector::getCachedStackWithAge(
    size_t rnInstanceId) const {
  auto lock = std::lock_guard(m_cacheMutex);
  auto it = m_cacheByInstance.find(rnInstanceId);
  if (it == m_cacheByInstance.end() || !it->second.hasValue ||
      it->second.stack.empty()) {
    return {"", -1};
  }

  const auto ageMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - it->second.capturedAt)
          .count();
  return {it->second.stack, ageMs};
}

std::string FreezeStackCollector::getCachedStack(size_t rnInstanceId) const {
  return getCachedStackWithAge(rnInstanceId).stack;
}

void FreezeStackCollector::clearCache(size_t rnInstanceId) {
  auto lock = std::lock_guard(m_cacheMutex);
  m_cacheByInstance.erase(rnInstanceId);
}

#if USE_HERMES
std::string FreezeStackCollector::formatProfilerSample(
    const facebook::hermes::sampling_profiler::ProfileSample& sample) {
  std::string result;
  result.reserve(sample.getCallStackFramesCount() * 64);

  for (const auto& frame : sample.getCallStackFramesRange()) {
    const auto* jsFrame =
        std::get_if<facebook::hermes::sampling_profiler::
                        ProfileSampleCallStackJSFunctionFrame>(&frame);
    if (jsFrame == nullptr) {
      continue;
    }

    const auto functionName = jsFrame->getFunctionName();
    if (functionName.empty()) {
      result.append("<anonymous>");
    } else {
      appendStringView(result, functionName);
    }

    const auto hasLocation = jsFrame->hasScriptUrl() ||
        jsFrame->hasFunctionLineNumber() || jsFrame->hasFunctionColumnNumber();
    if (hasLocation) {
      result.push_back('@');
      if (jsFrame->hasScriptUrl()) {
        appendStringView(result, jsFrame->getScriptUrl());
      } else {
        result.append("<unknown>");
      }
      if (jsFrame->hasFunctionLineNumber()) {
        result.push_back(':');
        result.append(std::to_string(jsFrame->getFunctionLineNumber()));
      }
      if (jsFrame->hasFunctionColumnNumber()) {
        result.push_back(':');
        result.append(std::to_string(jsFrame->getFunctionColumnNumber()));
      }
    }

    result.push_back('\n');
  }

  return result;
}

std::string FreezeStackCollector::captureStackFromProfiler(
    size_t rnInstanceId) const {
  auto lock = std::lock_guard(m_runtimeMutex);
  auto it = m_runtimeInfos.find(rnInstanceId);
  if (it == m_runtimeInfos.end() || it->second.hermesRuntime == nullptr) {
    return "";
  }

  try {
    auto* iHermes =
        static_cast<facebook::hermes::IHermes*>(it->second.hermesRuntime);
    auto profile = iHermes->dumpSampledTraceToProfile();
    auto samples = profile.getSamplesRange();

    for (auto it = samples.end(); it != samples.begin();) {
      --it;
      auto stack = formatProfilerSample(*it);
      if (!stack.empty()) {
        return stack;
      }
    }
  } catch (...) {
  }

  return "";
}
#else
std::string FreezeStackCollector::captureStackFromProfiler(
    size_t rnInstanceId) const {
  (void)rnInstanceId;
  return "";
}
#endif

} // namespace rnoh
