/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ComponentInstanceProvider.h"
#include <cxxreact/TraceSection.h>

using namespace rnoh;

ComponentInstanceProvider::ComponentInstanceProvider(
    ComponentInstancePreallocationRequestQueue::Shared
        preallocationRequestQueue,
    ComponentInstanceFactory::Shared componentInstanceFactory,
    ComponentInstanceRegistry::Shared componentInstanceRegistry,
    UITicker::Shared uiTicker,
    TaskExecutor::Weak weakTaskExecutor)
    : m_componentInstanceFactory(std::move(componentInstanceFactory)),
      m_preallocationRequestQueue(std::move(preallocationRequestQueue)),
      m_componentInstanceRegistry(std::move(componentInstanceRegistry)),
      m_uiTicker(std::move(uiTicker)),
      m_weakTaskExecutor(std::move(weakTaskExecutor)) {}

void ComponentInstanceProvider::initialize() {
  m_preallocationRequestQueue->setDelegate(this->weak_from_this());
}

ComponentInstanceProvider::~ComponentInstanceProvider() {
  m_threadGuard.assertThread();
  LOG(INFO) << "~ComponentInstanceProvider";
  std::lock_guard lock(m_unsubscribeUITickerListenerMtx);
  if (m_unsubscribeUITickerListener != nullptr) {
    m_unsubscribeUITickerListener();
  }
}

/**
 * @thread: JS
 */
void ComponentInstanceProvider::onPushPreallocationRequest() {
  std::lock_guard lock(this->m_unsubscribeUITickerListenerMtx);
  if (m_unsubscribeUITickerListener != nullptr) {
    return;
  }
  m_unsubscribeUITickerListener = m_uiTicker->subscribe(
      [weakSelf = this->weak_from_this(),
       weakTaskExecutor = this->m_weakTaskExecutor](auto recentVSyncTimestamp) {
        auto taskExecutor = weakTaskExecutor.lock();
        if (taskExecutor == nullptr) {
          return;
        }
        taskExecutor->runTask(
            TaskThread::MAIN, [weakSelf, recentVSyncTimestamp] {
              auto self = weakSelf.lock();
              if (self == nullptr) {
                return;
              }
              self->onUITick(recentVSyncTimestamp);
            });
      });
}

ComponentInstance::Shared ComponentInstanceProvider::getComponentInstance(
    facebook::react::Tag tag,
    facebook::react::ComponentHandle componentHandle,
    std::string componentName) {
  // Check the cache under the lock; release it before calling the factory.
  // create() is external/unknown code — holding the mutex across it risks
  // deadlock if the factory re-enters this provider, and serializes all
  // component creation, defeating the parallelization goal.
  {
    std::lock_guard<std::mutex> lock(m_preallocatedComponentInstanceByTagMtx);
    auto it = m_preallocatedComponentInstanceByTag.find(tag);
    if (it != m_preallocatedComponentInstanceByTag.end()) {
      return it->second;
    }
  }
  auto componentInstance = m_componentInstanceFactory->create(
      tag, componentHandle, std::move(componentName));
  // Re-acquire the lock to publish the new instance. Another thread may have
  // created one for the same tag in the meantime; keep the first inserted and
  // discard the duplicate so every caller observes the same instance.
  {
    std::lock_guard<std::mutex> lock(m_preallocatedComponentInstanceByTagMtx);
    auto [it, inserted] = m_preallocatedComponentInstanceByTag.try_emplace(
        tag, componentInstance);
    return it->second;
  }
}

bool ComponentInstanceProvider::hasComponentInstance(facebook::react::Tag tag) {
  std::lock_guard<std::mutex> lock(m_preallocatedComponentInstanceByTagMtx);
  auto componentInstanceIt = m_preallocatedComponentInstanceByTag.find(tag);
  return componentInstanceIt != m_preallocatedComponentInstanceByTag.end();
}

ComponentInstance::Shared ComponentInstanceProvider::createArkTSComponent(
    facebook::react::Tag tag,
    facebook::react::ComponentHandle componentHandle,
    std::string componentName) {
  m_threadGuard.assertThread();
  return m_componentInstanceFactory->createArkTSComponent(
      tag, componentHandle, std::move(componentName));
}

void rnoh::ComponentInstanceProvider::clearPreallocationRequestQueue() {
  m_preallocationRequestQueue->clear();
}

void rnoh::ComponentInstanceProvider::clearPreallocatedViews(
    facebook::react::ShadowViewMutationList mutations) {
  for (auto const& mutation : mutations) {
    std::lock_guard<std::mutex> lock(m_preallocatedComponentInstanceByTagMtx);
    m_preallocatedComponentInstanceByTag.erase(mutation.newChildShadowView.tag);
  }
}

void rnoh::ComponentInstanceProvider::clearPreallocatedViews() {
  std::lock_guard<std::mutex> lock(m_preallocatedComponentInstanceByTagMtx);
  m_preallocatedComponentInstanceByTag.clear();
}

void ComponentInstanceProvider::onUITick(
    UITicker::Timestamp recentVSyncTimestamp) {
  facebook::react::TraceSection s("ComponentInstanceProvider::onUITick");
  m_threadGuard.assertThread();
  if (m_preallocationRequestQueue->isEmpty()) {
    std::lock_guard lock(m_unsubscribeUITickerListenerMtx);
    if (m_unsubscribeUITickerListener != nullptr) {
      m_unsubscribeUITickerListener();
      m_unsubscribeUITickerListener = nullptr;
    }
    return;
  }
  while (true) {
    if (this->shouldPausePreallocationToAvoidBlockingMainThread(
            recentVSyncTimestamp)) {
      VLOG(2) << "Pausing preallocation to avoid blocking main thread";
      break;
    }
    auto maybeRequest = m_preallocationRequestQueue->pop();

    if (!maybeRequest.has_value()) {
      break;
    }
    processPreallocationRequest(maybeRequest.value());
  }
}

void ComponentInstanceProvider::processPreallocationRequest(
    ComponentInstancePreallocationRequestQueue::Request const& request) {
  bool isRequestOutdated =
      m_componentInstanceRegistry->findByTag(request.tag) != nullptr;
  if (isRequestOutdated) {
    return;
  }
  auto componentInstance = m_componentInstanceFactory->create(
      request.tag, request.componentHandle, request.componentName);
  if (componentInstance != nullptr) {
    std::lock_guard<std::mutex> lock(m_preallocatedComponentInstanceByTagMtx);
    m_preallocatedComponentInstanceByTag.emplace(
        request.tag, componentInstance);
  } else {
    VLOG(2) << "Couldn't preallocate CppComponentInstance for: "
            << request.componentName;
  }
}

bool ComponentInstanceProvider::
    shouldPausePreallocationToAvoidBlockingMainThread(
        UITicker::Timestamp recentVSyncTimestamp) {
  constexpr int FPS = 120;
  static constexpr auto FRAME_DURATION =
      std::chrono::nanoseconds(1000000000 / FPS);
  m_threadGuard.assertThread();
  auto timeLeftInFrame = FRAME_DURATION -
      (std::chrono::steady_clock::now() - recentVSyncTimestamp);
  return timeLeftInFrame < (FRAME_DURATION / 2);
}
