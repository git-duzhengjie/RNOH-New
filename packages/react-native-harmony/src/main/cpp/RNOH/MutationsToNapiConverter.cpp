/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "RNOH/MutationsToNapiConverter.h"
#include <cxxreact/TraceSection.h>
#include "MutationsToNapiConverter.h"
#include "RNOH/ArkJS.h"
#include "RNOH/BaseComponentNapiBinder.h"

using namespace facebook;
using namespace rnoh;

MutationsToNapiConverter::MutationsToNapiConverter(
    ComponentNapiBinderByString componentNapiBinderByName)
    : m_componentNapiBinderByName(std::move(componentNapiBinderByName)) {}

napi_value MutationsToNapiConverter::convert(
    napi_env env,
    react::ShadowViewMutationList const& mutations) const {
  facebook::react::TraceSection s("#RNOH::MutationsToNapiConverter::convert");
  std::vector<napi_value> napiMutations;
  ArkJS arkJS(env);
  for (auto& mutation : mutations) {
    auto objBuilder =
        arkJS.createObjectBuilder().addProperty("type", mutation.type);
    switch (mutation.type) {
      case react::ShadowViewMutation::Type::Create: {
        objBuilder.addProperty(
            "descriptor",
            this->convertShadowView(env, mutation.newChildShadowView));
        break;
      }
      case react::ShadowViewMutation::Type::Remove: {
        objBuilder.addProperty("childTag", mutation.oldChildShadowView.tag)
            .addProperty("parentTag", mutation.parentTag);
        break;
      }
      case react::ShadowViewMutation::Type::Update: {
        objBuilder.addProperty(
            "descriptor",
            this->convertShadowView(env, mutation.newChildShadowView));
        break;
      }
      case react::ShadowViewMutation::Type::Insert: {
        objBuilder.addProperty("childTag", mutation.newChildShadowView.tag)
            .addProperty("parentTag", mutation.parentTag)
            .addProperty("index", mutation.index);
        break;
      }
      case react::ShadowViewMutation::Type::Delete: {
        objBuilder.addProperty("tag", mutation.oldChildShadowView.tag);
        break;
      }
    }
    napiMutations.push_back(objBuilder.build());
  }
  return arkJS.createArray(napiMutations);
}

void rnoh::MutationsToNapiConverter::updateState(
    napi_env env,
    std::string const& componentName,
    facebook::react::State::Shared const& state,
    napi_value newState) {
  if (auto it = m_componentNapiBinderByName.find(componentName);
      it != m_componentNapiBinderByName.end()) {
    it->second->updateState({env, state, newState});
  }
  return;
}

napi_value MutationsToNapiConverter::convertShadowView(
    napi_env env,
    react::ShadowView const shadowView) const {
  ArkJS arkJS(env);
  auto descriptorBuilder = arkJS.createObjectBuilder();
  if (m_componentNapiBinderByName.count(shadowView.componentName) > 0) {
    const auto& componentNapiBinder =
        m_componentNapiBinderByName.at(shadowView.componentName);
    descriptorBuilder.addProperty("isDynamicBinder", arkJS.createBoolean(false))
        .addProperty("props", componentNapiBinder->createProps(env, shadowView))
        .addProperty(
            "state", componentNapiBinder->createState(env, shadowView));
  } else {
    BaseComponentNapiBinder baseNapiBinder;
    descriptorBuilder.addProperty("isDynamicBinder", arkJS.createBoolean(true))
        .addProperty("props", baseNapiBinder.createProps(env, shadowView))
        .addProperty("state", arkJS.createObjectBuilder().build());
  }
  descriptorBuilder.addProperty(
      "layoutMetrics",
      arkJS.createObjectBuilder()
          .addProperty(
              "frame",
              arkJS.createObjectBuilder()
                  .addProperty(
                      "origin",
                      arkJS.createObjectBuilder()
                          .addProperty(
                              "x", shadowView.layoutMetrics.frame.origin.x)
                          .addProperty(
                              "y", shadowView.layoutMetrics.frame.origin.y)
                          .build())
                  .addProperty(
                      "size",
                      arkJS.createObjectBuilder()
                          .addProperty(
                              "width",
                              shadowView.layoutMetrics.frame.size.width)
                          .addProperty(
                              "height",
                              shadowView.layoutMetrics.frame.size.height)
                          .build())
                  .build())
          .addProperty(
              "layoutDirection",
              static_cast<int>(shadowView.layoutMetrics.layoutDirection))
          .build());

  return descriptorBuilder.addProperty("tag", shadowView.tag)
      .addProperty("type", shadowView.componentName)
      .addProperty("childrenTags", arkJS.createArray())
      .addProperty(
          "rawProps", arkJS.createFromDynamic(shadowView.props->rawProps))
      .build();
}
