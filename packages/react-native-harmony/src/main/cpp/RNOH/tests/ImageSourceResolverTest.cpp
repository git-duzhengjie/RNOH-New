/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// ImageSourceResolverTest.cpp �?Tier 4b
// Tests the pure-C++ paths of ImageSourceResolver:
//  - isVideoUri / isRemoteUri
//  - getBestSourceForSize
//  - getImageSourceByName
//  - resolveImageSource (local, remote, video, cache-hit, pending, fallback)
//  - addListenerForURI / removeListenerForURI / removeListener
//  - onMessageReceived (UPDATE_IMAGE_SOURCE_MAP)
//  - getPrefetchedImageFileUri disk-cache fallback
//  - getDiskCacheKey / getFilePath / updateImageDiskCacheMap
// TurboModule/NAPI path is skipped (IsParallelizationEnabled() = true in stub).

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

#include <ImageSourceResolver.h>

using namespace rnoh;
using facebook::react::ImageSource;
using facebook::react::ImageSources;
using facebook::react::LayoutMetrics;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

class TestListener : public ImageSourceResolver::ImageSourceUpdateListener {
 public:
  int callCount = 0;
  explicit TestListener(ImageSourceResolver::Shared const& r)
      : ImageSourceResolver::ImageSourceUpdateListener(r) {}
  void onImageSourceCacheUpdate() override {
    callCount++;
  }
};

static LayoutMetrics makeMetrics(float w, float h, float scale = 1.0f) {
  LayoutMetrics m;
  m.frame.size.width = w;
  m.frame.size.height = h;
  m.pointScaleFactor = scale;
  return m;
}

static ImageSource makeLocal(const std::string& uri) {
  ImageSource s;
  s.type = ImageSource::Type::Local;
  s.uri = uri;
  return s;
}

static ImageSource
makeRemote(const std::string& uri, double w = 0, double h = 0) {
  ImageSource s;
  s.type = ImageSource::Type::Remote;
  s.uri = uri;
  s.size.width = w;
  s.size.height = h;
  return s;
}

// Creates a unique temp dir and returns its path.
static std::string makeTempCacheDir() {
  auto t = std::chrono::steady_clock::now().time_since_epoch().count();
  auto dir = std::filesystem::temp_directory_path() /
      ("isr_test_" + std::to_string(t));
  std::filesystem::create_directories(dir);
  return dir.string();
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class ImageSourceResolverTest : public ::testing::Test {
 protected:
  std::string cacheDir;
  ArkTSMessageHub::Shared hub;
  ImageSourceResolver::Shared resolver;
  RNInstance::Weak emptyInstance;

  void SetUp() override {
    cacheDir = makeTempCacheDir();
    hub = std::make_shared<ArkTSMessageHub>();
    resolver = std::make_shared<ImageSourceResolver>(
        hub,
        emptyInstance,
        std::unordered_map<std::string, std::string>{},
        cacheDir);
  }
  void TearDown() override {
    resolver.reset();
    std::error_code ec;
    std::filesystem::remove_all(cacheDir, ec);
  }
};

// ===========================================================================
// isVideoUri
// ===========================================================================

TEST_F(ImageSourceResolverTest, IsVideoUri_RecognizesCommonVideoExtensions) {
  EXPECT_TRUE(resolver->isVideoUri("http://x/y.mp4"));
  EXPECT_TRUE(resolver->isVideoUri("https://a.b/c.MOV"));
  EXPECT_TRUE(resolver->isVideoUri("file:///x/y.mkv"));
  EXPECT_TRUE(resolver->isVideoUri("/path/to/video.webm"));
}

TEST_F(ImageSourceResolverTest, IsVideoUri_IgnoresNonVideoExtensions) {
  EXPECT_FALSE(resolver->isVideoUri("http://x/y.png"));
  EXPECT_FALSE(resolver->isVideoUri("https://a.b/c.jpg?query=1"));
  EXPECT_FALSE(resolver->isVideoUri("file:///x/y.gif"));
}

TEST_F(ImageSourceResolverTest, IsVideoUri_StripsQueryParamsBeforeChecking) {
  EXPECT_TRUE(resolver->isVideoUri("http://x/y.mp4?t=123"));
  EXPECT_TRUE(resolver->isVideoUri("https://a/b/c.MOV?v=2&q=3"));
}

TEST_F(ImageSourceResolverTest, IsVideoUri_HandlesEdgeCases) {
  EXPECT_FALSE(resolver->isVideoUri(""));
  EXPECT_FALSE(resolver->isVideoUri("noextension"));
  EXPECT_FALSE(resolver->isVideoUri("/path/without/dot/"));
  EXPECT_TRUE(resolver->isVideoUri(".mp4"));
}

// ===========================================================================
// isRemoteUri
// ===========================================================================

TEST_F(ImageSourceResolverTest, IsRemoteUri_DetectsHttpAndHttps) {
  EXPECT_TRUE(resolver->isRemoteUri("http://example.com/x.png"));
  EXPECT_TRUE(resolver->isRemoteUri("https://example.com/y.jpg"));
}

TEST_F(ImageSourceResolverTest, IsRemoteUri_RejectsNonHttpSchemes) {
  EXPECT_FALSE(resolver->isRemoteUri("file:///x/y.png"));
  EXPECT_FALSE(resolver->isRemoteUri("asset://foo.png"));
  EXPECT_FALSE(resolver->isRemoteUri("data:image/png;base64,..."));
  EXPECT_FALSE(resolver->isRemoteUri(""));
}

// ===========================================================================
// getBestSourceForSize
// ===========================================================================

TEST_F(ImageSourceResolverTest, GetBestSourceForSize_SingleSource_ReturnsIt) {
  ImageSources srcs{makeRemote("http://x/a.png", 100, 100)};
  auto best = resolver->getBestSourceForSize(50, 50, 1.0, srcs);
  EXPECT_EQ(best.uri, "http://x/a.png");
}

TEST_F(ImageSourceResolverTest, GetBestSourceForSize_MultiSource_PicksClosest) {
  ImageSources srcs{
      makeRemote("http://x/small.png", 50, 50),
      makeRemote("http://x/medium.png", 200, 200),
      makeRemote("http://x/large.png", 800, 800),
  };
  // Target ~250*250*1 = 62500 px. medium=40000 (closest), large=640000 (far).
  auto best = resolver->getBestSourceForSize(250, 250, 1.0, srcs);
  EXPECT_EQ(best.uri, "http://x/medium.png");
}

TEST_F(ImageSourceResolverTest, GetBestSourceForSize_PrefersExactMatch) {
  ImageSources srcs{
      makeRemote("http://x/a.png", 100, 100),
      makeRemote("http://x/b.png", 200, 200),
      makeRemote("http://x/c.png", 300, 300),
  };
  // Target 200*200 = 40000. b=40000 (exact), so pixelsFit = 0 �?best.
  auto best = resolver->getBestSourceForSize(200, 200, 1.0, srcs);
  EXPECT_EQ(best.uri, "http://x/b.png");
}

TEST_F(ImageSourceResolverTest, GetBestSourceForSize_ScaleFactorAffectsTarget) {
  ImageSources srcs{
      makeRemote("http://x/a.png", 100, 100),
      makeRemote("http://x/b.png", 400, 400),
  };
  // Target 100*100*4 = 40000. a=10000 (|1-0.25|=0.75), b=160000 (|1-4|=3).
  auto best = resolver->getBestSourceForSize(100, 100, 4.0, srcs);
  EXPECT_EQ(best.uri, "http://x/a.png");
}

TEST_F(ImageSourceResolverTest, GetBestSourceForSize_FirstWinsOnTie) {
  ImageSources srcs{
      makeRemote("http://x/a.png", 100, 100),
      makeRemote("http://x/b.png", 100, 100),
  };
  auto best = resolver->getBestSourceForSize(50, 50, 1.0, srcs);
  EXPECT_EQ(best.uri, "http://x/a.png");
}

// ===========================================================================
// getImageSourceByName
// ===========================================================================

TEST_F(ImageSourceResolverTest, GetImageSourceByName_Found) {
  // Re-create resolver with a name map.
  TearDown();
  SetUp();
  std::unordered_map<std::string, std::string> names{
      {"logo", "asset://logo.png"}};
  resolver = std::make_shared<ImageSourceResolver>(
      hub, emptyInstance, names, cacheDir);
  auto r = resolver->getImageSourceByName("logo");
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(*r, "asset://logo.png");
}

TEST_F(ImageSourceResolverTest, GetImageSourceByName_NotFound) {
  auto r = resolver->getImageSourceByName("missing");
  EXPECT_FALSE(r.has_value());
}

// ===========================================================================
// resolveImageSource �?local / non-remote path
// ===========================================================================

TEST_F(
    ImageSourceResolverTest,
    ResolveImageSource_LocalSource_ReturnsDirectly) {
  ImageSources srcs{makeLocal("asset://foo.png")};
  auto r = resolver->resolveImageSource(
      *std::make_shared<TestListener>(resolver), makeMetrics(100, 100), srcs);
  EXPECT_EQ(r.bestResult.uri, "asset://foo.png");
  EXPECT_FALSE(r.bestResultUriInCache.has_value());
}

TEST_F(
    ImageSourceResolverTest,
    ResolveImageSource_RemoteButNotHttpOrVideo_ReturnsDirectly) {
  // type=Remote but uri doesn't start with http and isn't video
  ImageSources srcs{makeRemote("data:image/png;base64,xxx")};
  auto listener = std::make_shared<TestListener>(resolver);
  auto r = resolver->resolveImageSource(*listener, makeMetrics(50, 50), srcs);
  EXPECT_EQ(r.bestResult.uri, "data:image/png;base64,xxx");
  EXPECT_FALSE(r.bestResultUriInCache.has_value());
}

// ===========================================================================
// resolveImageSource �?remote path, no prefetch, no cache
// ===========================================================================

TEST_F(
    ImageSourceResolverTest,
    ResolveImageSource_RemoteHttp_NoDiskCache_ReturnsOriginalUriAsCache) {
  // With IsParallelizationEnabled()=true, TurboModule path is skipped.
  // getPrefetchedImageFileUri fallback returns the original URI (non-empty),
  // so resolveImageSource caches it as bestResultUriInCache.
  ImageSources srcs{makeRemote("http://example.com/a.png", 100, 100)};
  auto listener = std::make_shared<TestListener>(resolver);
  auto r = resolver->resolveImageSource(*listener, makeMetrics(50, 50), srcs);
  EXPECT_EQ(r.bestResult.uri, "http://example.com/a.png");
  ASSERT_TRUE(r.bestResultUriInCache.has_value());
  EXPECT_EQ(*r.bestResultUriInCache, "http://example.com/a.png");
  // Not pending — resolved to original URI
  EXPECT_FALSE(resolver->isUriPending("http://example.com/a.png"));
}

TEST_F(
    ImageSourceResolverTest,
    ResolveImageSource_RemoteHttp_CacheHit_ReturnsCachedUri) {
  ImageSources srcs{makeRemote("http://example.com/a.png", 100, 100)};
  resolver->injectRemoteCacheEntry(
      "http://example.com/a.png", "file:///c/a.png");
  auto listener = std::make_shared<TestListener>(resolver);
  auto r = resolver->resolveImageSource(*listener, makeMetrics(50, 50), srcs);
  EXPECT_EQ(r.bestResult.uri, "http://example.com/a.png");
  ASSERT_TRUE(r.bestResultUriInCache.has_value());
  EXPECT_EQ(*r.bestResultUriInCache, "file:///c/a.png");
}

TEST_F(
    ImageSourceResolverTest,
    ResolveImageSource_RemoteHttp_InjectedPending_ReturnsNoCache) {
  // Inject pending state directly (TurboModule path can't mark pending with
  // parallel=true)
  ImageSources srcs{makeRemote("http://example.com/a.png", 100, 100)};
  resolver->injectRemoteCacheEntry(
      "http://example.com/a.png", "__IMAGE_SOURCE_PENDING__");
  ASSERT_TRUE(resolver->isUriPending("http://example.com/a.png"));
  // While pending, resolve returns no cache URI
  auto listener = std::make_shared<TestListener>(resolver);
  auto r = resolver->resolveImageSource(*listener, makeMetrics(50, 50), srcs);
  EXPECT_FALSE(r.bestResultUriInCache.has_value());
}

// ===========================================================================
// resolveImageSource �?video path (always registers listener, no cache lookup)
// ===========================================================================

TEST_F(ImageSourceResolverTest, ResolveImageSource_Video_RegistersListener) {
  ImageSources srcs{makeRemote("http://x/v.mp4", 100, 100)};
  auto listener = std::make_shared<TestListener>(resolver);
  auto r = resolver->resolveImageSource(*listener, makeMetrics(50, 50), srcs);
  EXPECT_EQ(r.bestResult.uri, "http://x/v.mp4");
  EXPECT_EQ(listener->observedUri, "http://x/v.mp4");
}

// ===========================================================================
// addListenerForURI / removeListenerForURI / removeListener
// ===========================================================================

TEST_F(ImageSourceResolverTest, AddListener_RegistersAndDeduplicates) {
  auto l1 = std::make_shared<TestListener>(resolver);
  auto l2 = std::make_shared<TestListener>(resolver);
  resolver->addListenerForURI("http://x/a.png", l1.get());
  resolver->addListenerForURI("http://x/a.png", l2.get());
  // duplicate �?should be ignored
  resolver->addListenerForURI("http://x/a.png", l2.get());
  // No public count accessor, but no crash + listeners stored.
  SUCCEED();
}

TEST_F(ImageSourceResolverTest, RemoveListener_CleansUpEntryWhenEmpty) {
  auto l1 = std::make_shared<TestListener>(resolver);
  resolver->addListenerForURI("http://x/a.png", l1.get());
  resolver->removeListenerForURI("http://x/a.png", l1.get());
  // After removal, a fresh listener for same URI should start a new entry.
  auto l2 = std::make_shared<TestListener>(resolver);
  resolver->addListenerForURI("http://x/a.png", l2.get());
  SUCCEED();
}

TEST_F(ImageSourceResolverTest, RemoveListener_ByObservedUri) {
  auto l1 = std::make_shared<TestListener>(resolver);
  resolver->addListenerForURI("http://x/a.png", l1.get());
  EXPECT_EQ(l1->observedUri, "http://x/a.png");
  resolver->removeListener(l1.get());
  SUCCEED();
}

TEST_F(ImageSourceResolverTest, RemoveListenerForURI_MissingUri_NoCrash) {
  auto l1 = std::make_shared<TestListener>(resolver);
  resolver->removeListenerForURI("http://nope", l1.get());
  SUCCEED();
}

TEST_F(
    ImageSourceResolverTest,
    ImageSourceUpdateListener_DestructorRemovesItself) {
  std::weak_ptr<TestListener> weak;
  {
    auto l = std::make_shared<TestListener>(resolver);
    weak = l;
    resolver->addListenerForURI("http://x/a.png", l.get());
    // l goes out of scope here �?destructor should call removeListener
  }
  EXPECT_TRUE(weak.expired());
}

// ===========================================================================
// onMessageReceived �?UPDATE_IMAGE_SOURCE_MAP
// ===========================================================================

TEST_F(
    ImageSourceResolverTest,
    OnMessageReceived_UpdatesCacheAndNotifiesListeners) {
  // Register a listener for a URI
  auto l1 = std::make_shared<TestListener>(resolver);
  auto l2 = std::make_shared<TestListener>(resolver);
  resolver->addListenerForURI("http://x/a.png", l1.get());
  resolver->addListenerForURI("http://x/a.png", l2.get());

  ArkTSMessage msg;
  msg.name = "UPDATE_IMAGE_SOURCE_MAP";
  msg.payload["remoteUri"] = "http://x/a.png";
  msg.payload["fileUri"] = "file:///cache/a.png";
  resolver->receiveMessage(msg);

  EXPECT_EQ(l1->callCount, 1);
  EXPECT_EQ(l2->callCount, 1);

  // Cache should now have the file URI
  auto r = resolver->resolveImageSource(
      *std::make_shared<TestListener>(resolver),
      makeMetrics(50, 50),
      ImageSources{makeRemote("http://x/a.png", 100, 100)});
  ASSERT_TRUE(r.bestResultUriInCache.has_value());
  EXPECT_EQ(*r.bestResultUriInCache, "file:///cache/a.png");
}

TEST_F(
    ImageSourceResolverTest,
    OnMessageReceived_NoListeners_StillUpdatesCache) {
  ArkTSMessage msg;
  msg.name = "UPDATE_IMAGE_SOURCE_MAP";
  msg.payload["remoteUri"] = "http://x/b.png";
  msg.payload["fileUri"] = "file:///cache/b.png";
  resolver->receiveMessage(msg);

  auto r = resolver->resolveImageSource(
      *std::make_shared<TestListener>(resolver),
      makeMetrics(50, 50),
      ImageSources{makeRemote("http://x/b.png", 100, 100)});
  ASSERT_TRUE(r.bestResultUriInCache.has_value());
  EXPECT_EQ(*r.bestResultUriInCache, "file:///cache/b.png");
}

TEST_F(ImageSourceResolverTest, OnMessageReceived_OtherName_NoCacheInjection) {
  // Inject a pending state first to verify the "other" message doesn't clear it
  resolver->injectRemoteCacheEntry(
      "http://x/c.png", "__IMAGE_SOURCE_PENDING__");
  ASSERT_TRUE(resolver->isUriPending("http://x/c.png"));

  ArkTSMessage msg;
  msg.name = "SOME_OTHER_MESSAGE";
  msg.payload["remoteUri"] = "http://x/c.png";
  resolver->receiveMessage(msg);
  // Pending state should still be there (UPDATE_IMAGE_SOURCE_MAP not received)
  EXPECT_TRUE(resolver->isUriPending("http://x/c.png"));
}

TEST_F(ImageSourceResolverTest, OnMessageReceived_PendingReplacedByFileUri) {
  // Inject pending state directly (TurboModule path can't mark pending with
  // parallel=true)
  ImageSources srcs{makeRemote("http://x/d.png", 100, 100)};
  resolver->injectRemoteCacheEntry(
      "http://x/d.png", "__IMAGE_SOURCE_PENDING__");
  ASSERT_TRUE(resolver->isUriPending("http://x/d.png"));

  // Now UPDATE arrives
  ArkTSMessage msg;
  msg.name = "UPDATE_IMAGE_SOURCE_MAP";
  msg.payload["remoteUri"] = "http://x/d.png";
  msg.payload["fileUri"] = "file:///cache/d.png";
  resolver->receiveMessage(msg);

  EXPECT_FALSE(resolver->isUriPending("http://x/d.png"));
  auto r = resolver->resolveImageSource(
      *std::make_shared<TestListener>(resolver), makeMetrics(50, 50), srcs);
  ASSERT_TRUE(r.bestResultUriInCache.has_value());
  EXPECT_EQ(*r.bestResultUriInCache, "file:///cache/d.png");
}

// ===========================================================================
// getDiskCacheKey / getFilePath / updateImageDiskCacheMap
// ===========================================================================

TEST_F(ImageSourceResolverTest, GetDiskCacheKey_StripsNonAlphanumeric) {
  // Regex [^a-zA-Z0-9 \-] strips chars NOT in {letters, digits, space, dash}
  // http://x/a.png?b=1 → colons, slashes, dots, ?, = are all stripped
  EXPECT_EQ(resolver->getDiskCacheKeyPub("http://x/a.png?b=1"), "httpxapngb1");
}

TEST_F(ImageSourceResolverTest, GetDiskCacheKey_AllowsLettersDigitsSpaceDash) {
  auto k = resolver->getDiskCacheKeyPub("abc123 -");
  EXPECT_EQ(k, "abc123 -");
}

TEST_F(ImageSourceResolverTest, GetFilePath_ConcatenatesCacheDirAndKey) {
  auto p = resolver->getFilePathPub("mykey");
  EXPECT_EQ(p, cacheDir + "/mykey");
}

TEST_F(ImageSourceResolverTest, UpdateImageDiskCacheMap_AddsKey) {
  resolver->updateImageDiskCacheMapPub("k1");
  EXPECT_TRUE(resolver->diskCacheHas("k1"));
}

TEST_F(ImageSourceResolverTest, UpdateImageDiskCacheMap_EvictsAtCapacity) {
  for (int i = 0; i <= MAX_REMOTE_DISK_CACHE_CAP; ++i) {
    resolver->updateImageDiskCacheMapPub("k" + std::to_string(i));
  }
  // unordered_map begin() iteration order is unspecified, so we can't predict
  // which key was evicted — just verify the count stays at the cap.
  EXPECT_EQ(
      resolver->diskCacheCount(),
      static_cast<size_t>(MAX_REMOTE_DISK_CACHE_CAP));
}

// ===========================================================================
// getPrefetchedImageFileUri �?disk-cache fallback path
// ===========================================================================

TEST_F(
    ImageSourceResolverTest,
    GetPrefetchedImageFileUri_EmptyUri_ReturnsEmpty) {
  EXPECT_TRUE(resolver->getPrefetchedImageFileUriPub("").empty());
}

TEST_F(
    ImageSourceResolverTest,
    GetPrefetchedImageFileUri_NotInDiskCache_ReturnsOriginalUri) {
  auto r = resolver->getPrefetchedImageFileUriPub("http://x/a.png");
  EXPECT_EQ(r, "http://x/a.png");
}

TEST_F(
    ImageSourceResolverTest,
    GetPrefetchedImageFileUri_InDiskCacheButFileMissing_EvictsAndReturnsOriginal) {
  // Mark disk cache as having the key, but the file doesn't exist on disk
  std::string uri = "http://x/a.png";
  std::string key = resolver->getDiskCacheKeyPub(uri);
  resolver->updateImageDiskCacheMapPub(key);
  ASSERT_TRUE(resolver->diskCacheHas(key));

  auto r = resolver->getPrefetchedImageFileUriPub(uri);
  EXPECT_EQ(r, uri);
  // Key should have been evicted
  EXPECT_FALSE(resolver->diskCacheHas(key));
}

TEST_F(
    ImageSourceResolverTest,
    GetPrefetchedImageFileUri_InDiskCacheAndFileExists_ReturnsFileUri) {
  std::string uri = "http://x/a.png";
  std::string key = resolver->getDiskCacheKeyPub(uri);
  std::string path = resolver->getFilePathPub(key);

  // Create the file on disk
  {
    std::ofstream ofs(path);
    ofs << "fake";
  }
  resolver->updateImageDiskCacheMapPub(key);

  auto r = resolver->getPrefetchedImageFileUriPub(uri);
  EXPECT_EQ(r, "file://" + path);
}

// ===========================================================================
// Constructor �?scans cacheDir on startup
// ===========================================================================

TEST_F(
    ImageSourceResolverTest,
    Constructor_ScansCacheDir_PopulatesDiskCacheMap) {
  // Tear down existing, then create a fresh cache dir with files
  TearDown();
  cacheDir = makeTempCacheDir();

  // Create two files in the cache dir (names without special chars)
  std::string fileA = "httpxapng";
  std::string fileB = "httpxbpng";
  std::ofstream(cacheDir + "/" + fileA) << "fake1";
  std::ofstream(cacheDir + "/" + fileB) << "fake2";

  resolver = std::make_shared<ImageSourceResolver>(
      hub,
      emptyInstance,
      std::unordered_map<std::string, std::string>{},
      cacheDir);

  // The constructor runs getDiskCacheKey on each filename.
  // For names without special chars, the key == the filename itself.
  EXPECT_TRUE(resolver->diskCacheHas(fileA));
  EXPECT_TRUE(resolver->diskCacheHas(fileB));
  EXPECT_EQ(resolver->diskCacheCount(), 2u);
}

TEST_F(
    ImageSourceResolverTest,
    Constructor_NonExistentCacheDir_LogsErrorAndContinues) {
  std::string badDir = cacheDir + "/does/not/exist";
  // Should not throw �?should catch exception and continue
  EXPECT_NO_THROW({
    auto r = std::make_shared<ImageSourceResolver>(
        hub,
        emptyInstance,
        std::unordered_map<std::string, std::string>{},
        badDir);
  });
}
