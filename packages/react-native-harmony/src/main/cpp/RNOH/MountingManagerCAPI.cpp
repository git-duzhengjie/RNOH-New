/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "MountingManagerCAPI.h"
#include <cxxreact/TraceSection.h>
#include <react/renderer/components/image/ImageShadowNode.h>
#include <react/renderer/components/rncore/ShadowNodes.h>
#include <react/renderer/components/root/RootShadowNode.h>
#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>
#include <react/renderer/components/text/ParagraphShadowNode.h>
#include <react/renderer/components/text/RawTextShadowNode.h>
#include <react/renderer/components/text/TextShadowNode.h>
#include <react/renderer/components/textinput/TextInputShadowNode.h>
#include <react/renderer/components/view/ViewShadowNode.h>
#include "ParallelCheck.h"
#include "RNOH/Performance/RNOHMarker.h"
#include "RNOH/arkui/NativeNodeApi.h"

namespace rnoh {

std::string_view getMutationNameFromType(
    facebook::react::ShadowViewMutation::Type mutationType) {
  switch (mutationType) {
    case facebook::react::ShadowViewMutation::Create:
      return "CREATE";
    case facebook::react::ShadowViewMutation::Delete:
      return "DELETE";
    case facebook::react::ShadowViewMutation::Update:
      return "UPDATE";
    case facebook::react::ShadowViewMutation::Insert:
      return "INSERT";
    case facebook::react::ShadowViewMutation::Remove:
      return "REMOVE";
    default:
      return "UNKNOWN";
  }
}

std::optional<facebook::react::SurfaceId> findSurfaceIdForComponentInstance(
    ComponentInstance::Shared componentInstance) {
  auto parentInstance = componentInstance->getParent().lock();
  while (parentInstance != nullptr) {
    componentInstance = std::move(parentInstance);
    parentInstance = componentInstance->getParent().lock();
  }
  auto rootComponentTag = componentInstance->getTag();

  // Fabric reserves numbers equal 1 mod 10 for SurfaceIds
  if (rootComponentTag % 10 == 1) {
    return static_cast<facebook::react::SurfaceId>(rootComponentTag);
  }

  return std::nullopt;
}

void MountingManagerCAPI::willMount(MutationList const& /*mutations*/) {}

void MountingManagerCAPI::doMount(MutationList const& mutations) {
  facebook::react::TraceSection s("#RNOH::MountingManager::doMount");
  m_arkTSMountingManager->doMount(mutations);
}

void MountingManagerCAPI::didMount(MutationList const& mutations) {
  facebook::react::TraceSection s(
      ("#RNOH::MountingManager::didMount " + std::to_string(mutations.size()))
          .c_str());
  if (!m_featureFlagRegistry->isFeatureFlagOn(
          "PARTIAL_SYNC_OF_DESCRIPTOR_REGISTRY")) {
    m_arkTSMountingManager->didMount(mutations);
  } else {
    m_arkTSMountingManager->didMount(getArkTSMutations(mutations));
  }

  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::FABRIC_BATCH_EXECUTION_START);
  m_componentInstanceProvider->clearPreallocationRequestQueue();

  for (uint64_t i = 0; i < mutations.size(); i++) {
    auto const& mutation = mutations[i];
    try {
      this->handleMutation(mutation);
    } catch (std::exception const& e) {
      LOG(ERROR) << "Mutation " << getMutationNameFromType(mutation.type)
                 << " failed: " << e.what();
    }
  }
  this->finalizeMutationUpdates(mutations);
  RNOHMarker::logMarker(RNOHMarker::RNOHMarkerId::FABRIC_BATCH_EXECUTION_END);
}

void MountingManagerCAPI::dispatchCommand(
    const facebook::react::ShadowView& shadowView,
    const std::string& commandName,
    folly::dynamic const& args) {
  facebook::react::TraceSection s(
      "#RNOH::MountingManager::dispatchCommand ", shadowView.tag, commandName);
  if (m_arkTSComponentNames.count(shadowView.componentName)) {
    m_arkTSMountingManager->dispatchCommand(shadowView, commandName, args);
  }
  auto componentInstance =
      m_componentInstanceRegistry->findByTag(shadowView.tag);
  if (componentInstance) {
    componentInstance->handleCommand(commandName, args);
  }
}

void MountingManagerCAPI::setIsJsResponder(
    const facebook::react::ShadowView& shadowView,
    bool isJsResponder,
    bool blockNativeResponder) {
  if (m_arkTSComponentNames.count(shadowView.componentName)) {
    m_arkTSMountingManager->setIsJsResponder(
        shadowView, isJsResponder, blockNativeResponder);
  }

  auto componentInstance =
      m_componentInstanceRegistry->findByTag(shadowView.tag);
  if (componentInstance == nullptr) {
    return;
  }
  componentInstance->setIsJSResponder(isJsResponder);
  while (componentInstance != nullptr) {
    componentInstance->setNativeResponderBlocked(blockNativeResponder);
    componentInstance = componentInstance->getParent().lock();
  }
}

void MountingManagerCAPI::updateView(
    facebook::react::Tag tag,
    folly::dynamic props,
    facebook::react::ComponentDescriptor const& componentDescriptor) {
  facebook::react::TraceSection s("RNOH::MountingManagerCAPI::updateView");
  auto componentInstance = m_componentInstanceRegistry->findByTag(tag);
  if (componentInstance == nullptr) {
    return;
  }

  auto surfaceId = findSurfaceIdForComponentInstance(componentInstance);
  if (!surfaceId.has_value()) {
    return;
  }

  std::unordered_set<std::string> propKeys =
      componentInstance->getIgnoredPropKeys();
  for (const auto& key : props.keys()) {
    propKeys.insert(key.getString());
  }

  auto oldProps = componentInstance->getProps();
  auto newProps = componentDescriptor.cloneProps(
      facebook::react::PropsParserContext{
          surfaceId.value(), *m_contextContainer},
      oldProps,
      RawProps(props));

  componentInstance->setIgnoredPropKeys({});
  componentInstance->setProps(newProps);
  componentInstance->finalizeUpdates();
  componentInstance->setIgnoredPropKeys(std::move(propKeys));
}

void MountingManagerCAPI::updateComponentWithShadowView(
    ComponentInstance::Shared const& componentInstance,
    facebook::react::ShadowView const& shadowView) {
  // NOTE: updating tag by id must happen before updating props
  m_componentInstanceRegistry->updateTagById(
      shadowView.tag, shadowView.props->nativeId, componentInstance->getId());
  componentInstance->setShadowView(shadowView);
  componentInstance->setLayout(shadowView.layoutMetrics);
  componentInstance->setEventEmitter(shadowView.eventEmitter);
  componentInstance->setState(shadowView.state);
  componentInstance->setProps(shadowView.props);
}

const std::unordered_set<facebook::react::ComponentHandle>&
MountingManagerCAPI::getSupportedHandles() {
  static const std::unordered_set<facebook::react::ComponentHandle>
      supportedHandles = {
          facebook::react::ParagraphShadowNode::Handle(),
          facebook::react::TextShadowNode::Handle(),
          facebook::react::RawTextShadowNode::Handle(),
          facebook::react::ViewShadowNode::Handle(),
          facebook::react::ImageShadowNode::Handle(),
          facebook::react::RootShadowNode::Handle(),
          facebook::react::TextInputShadowNode::Handle(),
          facebook::react::ScrollViewShadowNode::Handle(),
          facebook::react::SwitchShadowNode::Handle(),
          facebook::react::PullToRefreshViewShadowNode::Handle(),
          facebook::react::ActivityIndicatorViewShadowNode::Handle(),
      };
  return supportedHandles;
}

bool MountingManagerCAPI::isComponentSupportedForParallelization(
    ComponentInstance::Shared const& componentInstance) const {
  if (getSupportedHandles().find(componentInstance->getComponentHandle()) !=
      getSupportedHandles().end()) {
    return true;
  }

  static const std::string FAST_IMAGE_VIEW = "FastImageView";
  return componentInstance->getComponentName() == FAST_IMAGE_VIEW;
}

void MountingManagerCAPI::handleMutation(Mutation const& mutation) {
  DVLOG(1) << "Mutation (type:" << getMutationNameFromType(mutation.type)
           << "; componentName: "
           << (mutation.newChildShadowView.componentName != nullptr
                   ? mutation.newChildShadowView.componentName
                   : "null")
           << "; newTag: " << mutation.newChildShadowView.tag
           << "; oldTag: " << mutation.oldChildShadowView.tag
           << "; parentTag: " << mutation.parentTag << ")";

  switch (mutation.type) {
    case facebook::react::ShadowViewMutation::Create: {
      const auto& newChild = mutation.newChildShadowView;
      auto componentInstance =
          m_componentInstanceProvider->getComponentInstance(
              newChild.tag, newChild.componentHandle, newChild.componentName);

      if (componentInstance == nullptr) {
        componentInstance = m_componentInstanceProvider->createArkTSComponent(
            newChild.tag, newChild.componentHandle, newChild.componentName);
      }

      if (componentInstance == nullptr) {
        LOG(ERROR) << "Couldn't create CppComponentInstance for: "
                   << newChild.componentName;
        return;
      }
      m_componentInstanceRegistry->insert(componentInstance);
      this->updateComponentWithShadowView(componentInstance, newChild);
      break;
    }
    case facebook::react::ShadowViewMutation::Delete: {
      const auto& oldChild = mutation.oldChildShadowView;
      m_componentInstanceRegistry->deleteByTag(oldChild.tag);
      break;
    }
    case facebook::react::ShadowViewMutation::Insert: {
      const auto& parentComponentInstance =
          m_componentInstanceRegistry->findByTag(mutation.parentTag);
      const auto& newChildComponentInstance =
          m_componentInstanceRegistry->findByTag(
              mutation.newChildShadowView.tag);

      if (parentComponentInstance != nullptr &&
          newChildComponentInstance != nullptr) {
        parentComponentInstance->insertChild(
            newChildComponentInstance, mutation.index);
        if (IsParallelizationEnabled() &&
            isComponentSupportedForParallelization(newChildComponentInstance)) {
          auto& node = newChildComponentInstance->getLocalRootArkUINode();
          auto arkHandle = node.getArkUINodeHandle();
          if (arkHandle) {
            const bool isParagraph =
                newChildComponentInstance->getComponentName() == "Paragraph";
            NativeNodeApi::getInstance()->markDirty(
                arkHandle,
                isParagraph ? ArkUI_NodeDirtyFlag::NODE_NEED_RENDER
                            : ArkUI_NodeDirtyFlag::NODE_NEED_MEASURE);
          }
        }
      }
      break;
    }
    case facebook::react::ShadowViewMutation::Remove: {
      const auto& parentComponentInstance =
          m_componentInstanceRegistry->findByTag(mutation.parentTag);
      if (parentComponentInstance) {
        const auto& childComponentInstance =
            m_componentInstanceRegistry->findByTag(
                mutation.oldChildShadowView.tag);
        parentComponentInstance->removeChild(childComponentInstance);
      }

      break;
    }
    case facebook::react::ShadowViewMutation::Update: {
      const auto& componentInstance = m_componentInstanceRegistry->findByTag(
          mutation.newChildShadowView.tag);
      if (componentInstance != nullptr) {
        this->updateComponentWithShadowView(
            componentInstance, mutation.newChildShadowView);
      }
      break;
    }
  }
}

void MountingManagerCAPI::finalizeMutationUpdates(
    MutationList const& mutations) {
  facebook::react::TraceSection s(
      "RNOH::MountingManagerCAPI::finalizeMutationUpdates");
  std::unordered_set<ComponentInstance::Shared> componentInstancesToFinalize;
  for (const auto& mutation : mutations) {
    std::shared_ptr<ComponentInstance> componentInstance = nullptr;
    switch (mutation.type) {
      case facebook::react::ShadowViewMutation::Create: {
        componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        break;
      }
      case facebook::react::ShadowViewMutation::Delete: {
        break;
      }
      case facebook::react::ShadowViewMutation::Insert: {
        componentInstance =
            m_componentInstanceRegistry->findByTag(mutation.parentTag);
        break;
      }
      case facebook::react::ShadowViewMutation::Remove: {
        componentInstance =
            m_componentInstanceRegistry->findByTag(mutation.parentTag);
        break;
      }
      case facebook::react::ShadowViewMutation::Update: {
        componentInstance = m_componentInstanceRegistry->findByTag(
            mutation.newChildShadowView.tag);
        break;
      }
    }
    if (componentInstance != nullptr) {
      if (componentInstancesToFinalize.find(componentInstance) ==
          componentInstancesToFinalize.end()) {
        componentInstance->finalizeUpdates();
        componentInstancesToFinalize.insert(componentInstance);
      }
    }
  }
}

auto MountingManagerCAPI::getArkTSMutations(MutationList const& mutations)
    -> MutationList {
  MutationList arkTSMutations{};
  for (auto const& mutation : mutations) {
    bool isArkTSMutation = false;
    switch (mutation.type) {
      case facebook::react::ShadowViewMutation::Create:
        isArkTSMutation = !isCAPIComponent(mutation.newChildShadowView);
        break;
      case facebook::react::ShadowViewMutation::Update:
        isArkTSMutation = !isCAPIComponent(mutation.newChildShadowView);
        break;
      case facebook::react::ShadowViewMutation::Delete:
        isArkTSMutation = !isCAPIComponent(mutation.oldChildShadowView);
        break;
      case facebook::react::ShadowViewMutation::Insert:
        isArkTSMutation = !isCAPIComponentByTag(mutation.parentTag) ||
            !isCAPIComponent(mutation.newChildShadowView);
        break;
      case facebook::react::ShadowViewMutation::Remove:
        isArkTSMutation = !isCAPIComponentByTag(mutation.parentTag) ||
            !isCAPIComponent(mutation.oldChildShadowView);
        break;
    }
    if (isArkTSMutation) {
      auto copyMutation = mutation;
      copyMutation.newChildShadowView.layoutMetrics.frame.origin = {0, 0};
      arkTSMutations.push_back(copyMutation);
    }
  }
  return arkTSMutations;
}

bool MountingManagerCAPI::isCAPIComponentByTag(facebook::react::Tag parentTag) {
  const auto& parentComponentInstance =
      m_componentInstanceRegistry->findByTag(parentTag);
  if (!parentComponentInstance) {
    return false;
  }
  std::string componentName = parentComponentInstance->getComponentName();
  facebook::react::ShadowView parentShadowView;
  parentShadowView.componentName = componentName.c_str();
  return isCAPIComponent(parentShadowView);
}

bool MountingManagerCAPI::isCAPIComponent(
    facebook::react::ShadowView const& shadowView) {
  std::string componentName = shadowView.componentName;
  if (m_cApiComponentNames.count(componentName) > 0 ||
      componentName == "RootView") {
    return true;
  }
  if (m_arkTSComponentNames.count(componentName) > 0) {
    return false;
  }
  auto isComponentPrecreated =
      m_componentInstanceProvider->hasComponentInstance(shadowView.tag);
  if (isComponentPrecreated) {
    m_cApiComponentNames.insert(std::move(componentName));
    return true;
  }
  m_arkTSComponentNames.insert(std::move(componentName));
  return false;
}

void MountingManagerCAPI::schedulerDidSendAccessibilityEvent(
    const facebook::react::ShadowView& shadowView,
    std::string const& eventType) {
  facebook::react::TraceSection s(
      "#RNOH::MountingManager::schedulerDidSendAccessibilityEvent");
  auto componentInstance =
      m_componentInstanceRegistry->findByTag(shadowView.tag);
  if (componentInstance == nullptr) {
    return;
  }
  folly::dynamic payload = folly::dynamic::object("type", eventType)(
      "targetId", componentInstance->getLocalRootArkUINode().getId());
  m_arkTSChannel->postMessage(
      "RNOH::schedulerDidSendAccessibilityEvent", payload);
};

void MountingManagerCAPI::clearPreallocatedViews() {
  m_componentInstanceProvider->clearPreallocatedViews();
}

void MountingManagerCAPI::clearPreallocatedViews(
    facebook::react::ShadowViewMutationList mutations) {
  m_componentInstanceProvider->clearPreallocatedViews(mutations);
}

} // namespace rnoh
