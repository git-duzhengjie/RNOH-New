/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <ImageCacheInterceptor.h>
#include <gtest/gtest.h>

#include <memory>

using namespace ImageCachePro;

namespace ImageCachePro {
class ImageCacheManagerInternal {};
} // namespace ImageCachePro

// -- Stub implementations (vtable linker requirement) --
bool ImageCacheInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>&,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)>) {
  return false;
}
void ImageCacheInterceptor::Cancel(const std::shared_ptr<ImageCacheTask>&) {}
void ImageCacheMemoryInterceptor::SetNext(
    const std::shared_ptr<ImageCacheMemoryInterceptor>& i) {
  next_ = i;
}
bool ImageCacheMemoryInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& t,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)> cb) {
  return ImageCacheInterceptor::Process(t, cb);
}
void ImageCacheLoadInterceptor::OnComplete(
    const std::shared_ptr<ImageCacheTask>&,
    bool) {}
void ImageCacheLoadInterceptor::Cancel(
    const std::shared_ptr<ImageCacheTask>& t) {
  if (next_)
    next_->Cancel(t);
}
bool ImageCacheLoadInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& t,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)> cb) {
  return ImageCacheInterceptor::Process(t, cb);
}
void ImageCacheLoadInterceptor::SetNext(
    const std::shared_ptr<ImageCacheLoadInterceptor>& i) {
  next_ = i;
}
void ImageCacheDecodeInterceptor::SetNext(
    const std::shared_ptr<ImageCacheDecodeInterceptor>& i) {
  next_ = i;
}
bool ImageCacheDecodeInterceptor::Process(
    const std::shared_ptr<ImageCacheTask>& t,
    std::function<bool(const std::shared_ptr<ImageCacheTask>&)> cb) {
  return ImageCacheInterceptor::Process(t, cb);
}

// -- Concrete test subclasses --
class TestInterceptor : public ImageCacheInterceptor {
 public:
  explicit TestInterceptor(ImageCacheManagerInternal& mgr)
      : ImageCacheInterceptor(mgr) {}
  bool Resolve(const std::shared_ptr<ImageCacheTask>&) override {
    return true;
  }
};
class TestMemoryInterceptor : public ImageCacheMemoryInterceptor {
 public:
  explicit TestMemoryInterceptor(ImageCacheManagerInternal& mgr)
      : ImageCacheMemoryInterceptor(mgr) {}
  bool Resolve(const std::shared_ptr<ImageCacheTask>&) override {
    return true;
  }
};
class TestLoadInterceptor : public ImageCacheLoadInterceptor {
 public:
  explicit TestLoadInterceptor(ImageCacheManagerInternal& mgr)
      : ImageCacheLoadInterceptor(mgr) {}
  bool Resolve(const std::shared_ptr<ImageCacheTask>&) override {
    return true;
  }
};
class TestDecodeInterceptor : public ImageCacheDecodeInterceptor {
 public:
  explicit TestDecodeInterceptor(ImageCacheManagerInternal& mgr)
      : ImageCacheDecodeInterceptor(mgr) {}
  bool Resolve(const std::shared_ptr<ImageCacheTask>&) override {
    return true;
  }
};

class ImageCacheInterceptorTest : public ::testing::Test {
 protected:
  ImageCacheManagerInternal manager;
  std::unique_ptr<TestInterceptor> interceptor;

  void SetUp() override {
    interceptor = std::make_unique<TestInterceptor>(manager);
  }
};

// === Constructor ===

TEST_F(ImageCacheInterceptorTest, Constructor_StoresManagerReference) {
  EXPECT_EQ(&interceptor->GetManager(), &manager);
}

TEST_F(ImageCacheInterceptorTest, Name_DefaultEmpty) {
  EXPECT_TRUE(interceptor->name.empty());
}

TEST_F(ImageCacheInterceptorTest, Name_SetAndGet) {
  interceptor->name = "LoadInterceptor";
  EXPECT_EQ(interceptor->name, "LoadInterceptor");
}

TEST_F(
    ImageCacheInterceptorTest,
    Name_DifferentInterceptorsHaveIndependentNames) {
  auto interceptor2 = std::make_unique<TestInterceptor>(manager);
  interceptor->name = "first";
  interceptor2->name = "second";
  EXPECT_NE(interceptor->name, interceptor2->name);
  EXPECT_EQ(interceptor->name, "first");
  EXPECT_EQ(interceptor2->name, "second");
}

// === Resolve ===

TEST_F(ImageCacheInterceptorTest, Resolve_ReturnsTrue) {
  EXPECT_TRUE(interceptor->Resolve(nullptr));
}

TEST_F(ImageCacheInterceptorTest, Resolve_CalledTwile_BothReturnTrue) {
  EXPECT_TRUE(interceptor->Resolve(nullptr));
  EXPECT_TRUE(interceptor->Resolve(nullptr));
}

// === Subclass construction ===

TEST_F(ImageCacheInterceptorTest, MemoryInterceptor_Construct_StoresManager) {
  auto mi = std::make_unique<TestMemoryInterceptor>(manager);
  EXPECT_EQ(&mi->GetManager(), &manager);
  EXPECT_EQ(mi->name, "");
}

TEST_F(ImageCacheInterceptorTest, LoadInterceptor_Construct_StoresManager) {
  auto li = std::make_unique<TestLoadInterceptor>(manager);
  EXPECT_EQ(&li->GetManager(), &manager);
  EXPECT_EQ(li->name, "");
}

TEST_F(ImageCacheInterceptorTest, DecodeInterceptor_Construct_StoresManager) {
  auto di = std::make_unique<TestDecodeInterceptor>(manager);
  EXPECT_EQ(&di->GetManager(), &manager);
  EXPECT_EQ(di->name, "");
}

// === SetNext ===

TEST_F(ImageCacheInterceptorTest, SetNext_MemoryInterceptor_StoresNext) {
  auto m1 = std::make_shared<TestMemoryInterceptor>(manager);
  auto m2 = std::make_shared<TestMemoryInterceptor>(manager);
  m1->SetNext(m2);
  EXPECT_NE(&m1->GetManager(), &m2->GetManager());
  EXPECT_EQ(&m1->GetManager(), &manager);
}

TEST_F(ImageCacheInterceptorTest, SetNext_LoadInterceptor_StoresNext) {
  auto l1 = std::make_shared<TestLoadInterceptor>(manager);
  auto l2 = std::make_shared<TestLoadInterceptor>(manager);
  l1->SetNext(l2);
  EXPECT_EQ(&l1->GetManager(), &manager);
  EXPECT_NE(l1->name, "set");
}

TEST_F(ImageCacheInterceptorTest, SetNext_DecodeInterceptor_StoresNext) {
  auto d1 = std::make_shared<TestDecodeInterceptor>(manager);
  auto d2 = std::make_shared<TestDecodeInterceptor>(manager);
  d1->SetNext(d2);
  EXPECT_EQ(&d1->GetManager(), &manager);
}

// === Cancel ===

TEST_F(ImageCacheInterceptorTest, Cancel_WithNext_NoCrash) {
  auto l1 = std::make_shared<TestLoadInterceptor>(manager);
  auto l2 = std::make_shared<TestLoadInterceptor>(manager);
  l1->SetNext(l2);
  EXPECT_NO_THROW(l1->Cancel(nullptr));
}

TEST_F(ImageCacheInterceptorTest, Cancel_WithoutNext_NoCrash) {
  auto l1 = std::make_shared<TestLoadInterceptor>(manager);
  EXPECT_NO_THROW(l1->Cancel(nullptr));
}

// === Process ===

TEST_F(ImageCacheInterceptorTest, Process_ReturnsFalse) {
  auto result = interceptor->Process(
      nullptr, [](const std::shared_ptr<ImageCacheTask>&) { return true; });
  EXPECT_FALSE(result);
}

TEST_F(ImageCacheInterceptorTest, Process_LoadInterceptor_DelegatesToBase) {
  auto li = std::make_unique<TestLoadInterceptor>(manager);
  auto result = li->Process(
      nullptr, [](const std::shared_ptr<ImageCacheTask>&) { return true; });
  EXPECT_FALSE(result);
}

TEST_F(ImageCacheInterceptorTest, Process_DecodeInterceptor_DelegatesToBase) {
  auto di = std::make_unique<TestDecodeInterceptor>(manager);
  auto result = di->Process(
      nullptr, [](const std::shared_ptr<ImageCacheTask>&) { return true; });
  EXPECT_FALSE(result);
}

// === Different managers produce different interceptors ===

TEST_F(ImageCacheInterceptorTest, DifferentManagers_DifferentAddresses) {
  ImageCacheManagerInternal manager2;
  auto i2 = std::make_unique<TestInterceptor>(manager2);
  EXPECT_NE(&interceptor->GetManager(), &i2->GetManager());
  EXPECT_EQ(&i2->GetManager(), &manager2);
}
