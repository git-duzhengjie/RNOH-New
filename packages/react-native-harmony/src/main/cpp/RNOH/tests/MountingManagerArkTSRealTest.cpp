/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "RNOH/MountingManagerArkTS.h"

using namespace rnoh;
using namespace facebook::react;

namespace rnoh::test {

class MockShadowViewRegistry : public ShadowViewRegistry {
 public:
  std::unordered_map<Tag, ShadowView> stored;
  int setCount = 0;
  int clearCount = 0;

  void setShadowView(Tag tag, ShadowView const& sv) override {
    stored[tag] = sv;
    setCount++;
  }
  void clearShadowView(Tag tag) override {
    stored.erase(tag);
    clearCount++;
  }
};

class MockArkTSChannel : public ArkTSChannel {
 public:
  std::string lastMsgName;
  folly::dynamic lastPayload;

  void postMessage(const std::string& name, folly::dynamic const& payload) override {
    lastMsgName = name;
    lastPayload = payload;
  }
};

static ShadowViewMutation makeMutation(ShadowViewMutation::Type type, Tag tag = 1) {
  ShadowViewMutation m;
  m.type = type;
  m.newChildShadowView.tag = tag;
  m.oldChildShadowView.tag = tag;
  return m;
}

static MountingManagerArkTS::Shared createManager(
    std::shared_ptr<MockShadowViewRegistry> registry,
    std::shared_ptr<MockArkTSChannel> channel,
    MountingManagerArkTS::TriggerUICallback uiCb = {},
    MountingManagerArkTS::CommandDispatcher cmdDisp = {}) {
  return std::make_shared<MountingManagerArkTS>(
      registry,
      uiCb ? uiCb : [](ShadowViewMutationList const&) {},
      cmdDisp ? cmdDisp : [](Tag, std::string const&, folly::dynamic const) {},
      channel);
}

} // namespace rnoh::test

// ============================================================================
// doMount 鈥?branches: for-loop + switch (5 cases)
// ============================================================================

TEST(MountingManagerArkTSReal, DoMount_EmptyMutations_NoRegistryCalls) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList empty;
  mgr->doMount(empty);

  EXPECT_EQ(reg->setCount, 0);
  EXPECT_EQ(reg->clearCount, 0);
}

TEST(MountingManagerArkTSReal, DoMount_CreateMutation_CallsSetShadowView) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList mutations = {test::makeMutation(ShadowViewMutation::Create, 42)};
  mgr->doMount(mutations);

  EXPECT_EQ(reg->setCount, 1);
  EXPECT_EQ(reg->clearCount, 0);
  EXPECT_TRUE(reg->stored.find(42) != reg->stored.end());
}

TEST(MountingManagerArkTSReal, DoMount_DeleteMutation_CallsClearShadowView) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList mutations = {test::makeMutation(ShadowViewMutation::Delete, 99)};
  mgr->doMount(mutations);

  EXPECT_EQ(reg->setCount, 0);
  EXPECT_EQ(reg->clearCount, 1);
}

TEST(MountingManagerArkTSReal, DoMount_InsertMutation_NoRegistryCalls) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList mutations = {test::makeMutation(ShadowViewMutation::Insert, 5)};
  mgr->doMount(mutations);

  EXPECT_EQ(reg->setCount, 0);
  EXPECT_EQ(reg->clearCount, 0);
}

TEST(MountingManagerArkTSReal, DoMount_RemoveMutation_NoRegistryCalls) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList mutations = {test::makeMutation(ShadowViewMutation::Remove, 7)};
  mgr->doMount(mutations);

  EXPECT_EQ(reg->setCount, 0);
  EXPECT_EQ(reg->clearCount, 0);
}

TEST(MountingManagerArkTSReal, DoMount_UpdateMutation_CallsSetShadowView) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList mutations = {test::makeMutation(ShadowViewMutation::Update, 77)};
  mgr->doMount(mutations);

  EXPECT_EQ(reg->setCount, 1);
  EXPECT_EQ(reg->clearCount, 0);
}

TEST(MountingManagerArkTSReal, DoMount_MixedMutations_CallsCorrectRegistryMethods) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowViewMutationList mutations = {
    test::makeMutation(ShadowViewMutation::Create, 1),
    test::makeMutation(ShadowViewMutation::Insert, 2),
    test::makeMutation(ShadowViewMutation::Update, 1),
    test::makeMutation(ShadowViewMutation::Remove, 2),
    test::makeMutation(ShadowViewMutation::Delete, 1)
  };
  mgr->doMount(mutations);

  EXPECT_EQ(reg->setCount, 2);
  EXPECT_EQ(reg->clearCount, 1);
}

// ============================================================================
// didMount 鈥?branch: if (mutations.size() > 0)
// ============================================================================

TEST(MountingManagerArkTSReal, DidMount_EmptyMutations_DoesNotCallCallback) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  bool called = false;
  auto mgr = test::createManager(reg, ch,
      [&](ShadowViewMutationList const&) { called = true; });

  ShadowViewMutationList empty;
  mgr->didMount(empty);

  EXPECT_FALSE(called);
}

TEST(MountingManagerArkTSReal, DidMount_NonEmptyMutations_CallsCallback) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  bool called = false;
  auto mgr = test::createManager(reg, ch,
      [&](ShadowViewMutationList const&) { called = true; });

  ShadowViewMutationList mutations = {test::makeMutation(ShadowViewMutation::Create, 1)};
  mgr->didMount(mutations);

  EXPECT_TRUE(called);
}

// ============================================================================
// dispatchCommand 鈥?no branches, verify delegation
// ============================================================================

TEST(MountingManagerArkTSReal, DispatchCommand_CallsCommandDispatcher) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  Tag receivedTag = -1;
  std::string receivedCmd;
  auto mgr = test::createManager(reg, ch, {},
      [&](Tag tag, std::string const& cmd, folly::dynamic const) {
        receivedTag = tag;
        receivedCmd = cmd;
      });

  ShadowView sv;
  sv.tag = 55;
  mgr->dispatchCommand(sv, "scrollTo", folly::dynamic::object("y", 100));

  EXPECT_EQ(receivedTag, 55);
  EXPECT_EQ(receivedCmd, "scrollTo");
}

// ============================================================================
// setIsJsResponder 鈥?verify postMessage delegation
// ============================================================================

TEST(MountingManagerArkTSReal, SetIsJsResponder_PostsMessageToChannel) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  ShadowView sv;
  sv.tag = 33;
  mgr->setIsJsResponder(sv, true, false);

  EXPECT_EQ(ch->lastMsgName, "SCHEDULER_DID_SET_IS_JS_RESPONDER");
  EXPECT_EQ(ch->lastPayload["tag"].getInt(), 33);
  EXPECT_TRUE(ch->lastPayload["isJSResponder"].getBool());
}

// ============================================================================
// updateView / clearPreallocatedViews 鈥?throw RNOHError
// ============================================================================

TEST(MountingManagerArkTSReal, UpdateView_ThrowsRNOHError) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  facebook::react::ComponentDescriptor cd;
  EXPECT_THROW(mgr->updateView(1, folly::dynamic::object(), cd), RNOHError);
}

TEST(MountingManagerArkTSReal, ClearPreallocatedViews_ThrowsRNOHError) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  EXPECT_THROW(mgr->clearPreallocatedViews(), RNOHError);
  EXPECT_THROW(mgr->clearPreallocatedViews({}), RNOHError);
}

// ============================================================================
// willMount / handleMutation / finalizeMutationUpdates 鈥?no-ops
// ============================================================================

TEST(MountingManagerArkTSReal, WillMount_DoesNotThrow) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  EXPECT_NO_THROW(mgr->willMount({}));
}

TEST(MountingManagerArkTSReal, HandleMutation_DoesNotThrow) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  EXPECT_NO_THROW(mgr->handleMutation(test::makeMutation(ShadowViewMutation::Create, 1)));
}

TEST(MountingManagerArkTSReal, FinalizeMutationUpdates_DoesNotThrow) {
  auto reg = std::make_shared<test::MockShadowViewRegistry>();
  auto ch = std::make_shared<test::MockArkTSChannel>();
  auto mgr = test::createManager(reg, ch);

  EXPECT_NO_THROW(mgr->finalizeMutationUpdates({}));
}
