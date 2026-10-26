/**
 * Copyright (c) 2024 Huawei Technologies Co., Ltd.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE-MIT file in the root directory of this source tree.
 */

#pragma once

#include <ReactCommon/RuntimeExecutor.h>
#include <react/renderer/imagemanager/primitives.h>
#include <filesystem>
#include <list>
#include <mutex>
#include <regex>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include "RNInstance.h"
#include "RNOH/ArkTSMessageHub.h"
#include "RNOH/Assert.h"
#include "RNOH/ParallelCheck.h"
#include "RNOH/RNInstance.h"
#include "RNOH/TaskExecutor/TaskExecutor.h"
#include "RNOH/TaskExecutor/ThreadTaskRunner.h"
#include "RNOHCorePackage/TurboModules/ImageLoaderTurboModule.h"

namespace rnoh {

constexpr int MAX_REMOTE_DISK_CACHE_CAP = 128;
// Return value of the TS-side getPrefetchResult when a prefetch is in-flight
// (used only by the video-thumbnail trigger path below).
constexpr std::string_view PREFETCH_PENDING = "pending";
static const std::unordered_set<std::string> VIDEO_FORMATS = {
    ".mp4", ".mov",  ".avi", ".mpeg", ".mpg",  ".mkv",  ".flv",
    ".wmv", ".m2ts", ".vob", ".ts",   ".webm", ".m3u8", ".mpd",
    ".3gp", ".3g2",  ".rm",  ".rmvb", ".f4v",  ".mxf"};

class ImageSourceResolver : public ArkTSMessageHub::Observer {
 public:
  using Shared = std::shared_ptr<ImageSourceResolver>;
  bool isVideoUri(const std::string& uri) {
    if (uri.empty()) {
      return false;
    }

    std::string path = uri;
    size_t paramIndex = uri.find('?');
    if (paramIndex != std::string::npos) {
      path = uri.substr(0, paramIndex);
    }

    size_t lastDot = path.find_last_of('.');
    if (lastDot == std::string::npos) {
      return false;
    }

    std::string suffix = path.substr(lastDot);
    std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);

    return VIDEO_FORMATS.find(suffix) != VIDEO_FORMATS.end();
  }

  bool isRemoteUri(const std::string& uri) {
    if (uri.empty()) {
      return false;
    }
    // Check if URI starts with "http" (matches both "http://" and "https://")
    return uri.find("http", 0) == 0;
  }

  ImageSourceResolver(
      ArkTSMessageHub::Shared const& subject,
      RNInstance::Weak rnInstance,
      std::unordered_map<std::string, std::string> imageSourceByName,
      std::string cacheDir)
      : ArkTSMessageHub::Observer(subject),
        m_rnInstance(rnInstance),
        imageSourceByName(imageSourceByName),
        m_cacheDir(std::move(cacheDir)) {
    // One-time startup scan building imageDiskCacheMap (cacheKey ->
    // filePath), read-only afterwards. m_cacheDir is app-level, so the first
    // instance wins.
    std::call_once(s_diskCacheScanOnce, [this] { scanDiskCache(); });
  }

  class ImageSourceUpdateListener {
   public:
    std::string observedUri;

    ImageSourceUpdateListener(
        ImageSourceResolver::Shared const& imageSourceResolver)
        : m_imageSourceResolver(imageSourceResolver){};

    ~ImageSourceUpdateListener() {
      m_imageSourceResolver->removeListener(this);
    }

    virtual void onImageSourceCacheUpdate() = 0;

   private:
    ImageSourceResolver::Shared const& m_imageSourceResolver;
  };

  // Based on Android MultiSourceHelper class, see:
  // https://github.com/facebook/react-native/blob/v0.77.1/packages/react-native/ReactAndroid/src/main/java/com/facebook/react/views/imagehelper/MultiSourceHelper.kt#L81
  struct MultiSourceResolutionResult {
    /**
     * Get the best result overall (closest in size to the view's size).
     */
    facebook::react::ImageSource bestResult;
    /**
     * Get the best result URI (closest in size to the view's size) that is also
     * in cache. If this would be the same as the URI in `bestResult`, this will
     * return `std::nullopt` instead.
     */
    std::optional<std::string> bestResultUriInCache;
  };

  MultiSourceResolutionResult resolveImageSource(
      ImageSourceUpdateListener& listener,
      facebook::react::LayoutMetrics const& layoutMetrics,
      facebook::react::ImageSources const& newSourcesCandidates) {
    auto imageCandidate = getBestSourceForSize(
        layoutMetrics.frame.size.width,
        layoutMetrics.frame.size.height,
        layoutMetrics.pointScaleFactor,
        newSourcesCandidates);

    if (imageCandidate.type != facebook::react::ImageSource::Type::Remote) {
      return {imageCandidate};
    }

    if (!isRemoteUri(imageCandidate.uri) && !isVideoUri(imageCandidate.uri)) {
      return {imageCandidate};
    }

    {
      // Subscribe to get information about prefetched URIs.
      std::lock_guard<std::mutex> lock(m_uriListenersMapMutex);
      if (isVideoUri(imageCandidate.uri)) {
        addListenerForURI(imageCandidate.uri, &listener);
      } else {
        if (uriListenersMap.find(imageCandidate.uri) == uriListenersMap.end()) {
          // m_uriListenersMapMutex is already held in this scope.
          removeListenerForURI(listener.observedUri, &listener);
          addListenerForURI(imageCandidate.uri, &listener);
        }
      }
    }

    // Read path: query remoteImageCache (uri -> fileUri).
    {
      auto fileUri = findValidCachedFileUri(imageCandidate.uri);
      if (fileUri.has_value()) {
        return {imageCandidate, fileUri.value()};
      }
    }

    // === VIDEO thumbnail trigger (non-parallel only) ===
    // Image cache lookups are fully C++-local (no callSync), but video
    // thumbnails are GENERATED on the TS side via media APIs. The trigger is
    // the legacy callSync("getPrefetchResult"): returns "pending" (kicks off
    // async generation) / a "file://" (TS videoThumbnailCache hit) /
    // undefined. Skipped in parallel mode (NAPI not thread-safe; matches the
    // 0.82 limitation — video thumbnails don't generate under parallelism).
    if (isVideoUri(imageCandidate.uri) && !IsParallelizationEnabled()) {
      assertMainThread();
      auto rnInstance = m_rnInstance.lock();
      if (rnInstance != nullptr) {
        auto imageLoaderTurboModule =
            rnInstance->getTurboModule<ImageLoaderTurboModule>("ImageLoader");
        if (imageLoaderTurboModule != nullptr) {
          auto prefetchResult = imageLoaderTurboModule->callSync(
              "getPrefetchResult", {imageCandidate.uri});
          if (!prefetchResult.isNull()) {
            auto prefetchResultString = prefetchResult.asString();
            if (prefetchResultString == PREFETCH_PENDING) {
              return {imageCandidate};
            }
            if (!prefetchResultString.empty()) {
              return {imageCandidate, prefetchResultString};
            }
          }
        }
      }
    }

    // Startup disk-index fallback (read-only).
    {
      auto cacheKey = getDiskCacheKey(imageCandidate.uri);
      auto dit = imageDiskCacheMap.find(cacheKey);
      std::error_code ec;
      if (dit != imageDiskCacheMap.end() &&
          std::filesystem::exists(dit->second, ec)) {
        std::string diskFileUri = "file://" + dit->second;
        remoteImageCache.set(imageCandidate.uri, diskFileUri);
        return {imageCandidate, diskFileUri};
      }
    }
    return {imageCandidate};
  }

  // Based on Android MultiSourceHelper class, see:
  // https://github.com/facebook/react-native/blob/v0.72.5/packages/react-native/ReactAndroid/src/main/java/com/facebook/react/views/imagehelper/MultiSourceHelper.java#L61
  facebook::react::ImageSource getBestSourceForSize(
      double width,
      double height,
      double pointScaleFactor,
      facebook::react::ImageSources const& sources) {
    RNOH_ASSERT_MSG(
        sources.size() > 0, "ImageSources vector should not be empty");

    if (sources.size() == 1) {
      return sources[0];
    }

    auto bestSourceIndex = 0;
    auto targetImagePixels = width * height * pointScaleFactor;
    auto bestPixelsFit = std::numeric_limits<double>::max();

    for (auto i = 0; i < sources.size(); i++) {
      auto& source = sources[i];
      auto imagePixels = source.size.width * source.size.height;
      auto pixelsFit = std::abs(1.0 - imagePixels / targetImagePixels);

      if (pixelsFit < bestPixelsFit) {
        bestPixelsFit = pixelsFit;
        bestSourceIndex = i;
      }
    }
    return sources[bestSourceIndex];
  }

  void addListenerForURI(
      const std::string& uri,
      ImageSourceUpdateListener* listener) {
    listener->observedUri = uri;
    auto it = uriListenersMap.find(uri);
    if (it == uriListenersMap.end()) {
      uriListenersMap.emplace(
          uri, std::initializer_list<ImageSourceUpdateListener*>{listener});
      return;
    }
    if (std::find(it->second.begin(), it->second.end(), listener) !=
        it->second.end()) {
      return;
    }
    it->second.push_back(listener);
  }

  void removeListenerForURI(
      const std::string& uri,
      ImageSourceUpdateListener* listener) {
    auto it = uriListenersMap.find(uri);
    if (it == uriListenersMap.end()) {
      return;
    }
    auto& listeners = it->second;
    auto listenerPos = std::find(listeners.begin(), listeners.end(), listener);
    if (listenerPos != listeners.end()) {
      listeners.erase(listenerPos);
      if (listeners.empty()) {
        uriListenersMap.erase(uri);
      }
    }
  }

  void removeListener(ImageSourceUpdateListener* listener) {
    std::lock_guard<std::mutex> lock(m_uriListenersMapMutex);
    removeListenerForURI(listener->observedUri, listener);
  }

  std::optional<std::string> getImageSourceByName(const std::string& name) {
    if (imageSourceByName.contains(name)) {
      return {imageSourceByName[name]};
    }
    return std::nullopt;
  }

 protected:
  virtual void onMessageReceived(const ArkTSMessage& message) override {
    if (message.name == "UPDATE_IMAGE_SOURCE_MAP") {
      auto remoteUri = message.payload["remoteUri"].asString();
      auto fileUri = message.payload["fileUri"].asString();
      // Pure "cache ready" signal: TS posts only after the file is written
      // (failures are not reported).
      if (remoteUri.empty() || fileUri.rfind("file://", 0) != 0) {
        return;
      }
      onCacheFileReady(remoteUri, fileUri);
    }
  }

 private:
  class RemoteImageFifoCache {
   public:
    std::optional<std::string> find(const std::string& uri) {
      std::shared_lock lock(m_mutex);
      auto it = m_map.find(uri);
      if (it == m_map.end()) {
        return std::nullopt;
      }
      return it->second.fileUri;
    }

    // Inserts or refreshes the uri -> fileUri mapping; evicts the oldest
    // entries (FIFO) beyond MAX_REMOTE_DISK_CACHE_CAP. Evicted files are
    // purged asynchronously after a grace period (DeferredFilePurger):
    // a file:// uri handed to a component milliseconds ago may not have
    // been opened by ArkUI's async image pipeline yet, and deleting the
    // file inline would blank that in-flight render.
    void set(const std::string& uri, const std::string& fileUri) {
      std::vector<std::pair<std::string, std::string>> evicted;
      {
        std::unique_lock lock(m_mutex);
        auto it = m_map.find(uri);
        if (it != m_map.end()) {
          // O(1) move-to-back; splice keeps fifoPos valid, no reassign needed.
          m_fifo.splice(m_fifo.end(), m_fifo, it->second.fifoPos);
          it->second.fileUri = fileUri;
        } else {
          auto fifoPos = m_fifo.insert(m_fifo.end(), uri);
          m_map.emplace(uri, Entry{fileUri, fifoPos});
        }
        while (m_fifo.size() > MAX_REMOTE_DISK_CACHE_CAP) {
          auto oldestUri = m_fifo.front();
          auto eit = m_map.find(oldestUri);
          if (eit != m_map.end()) {
            evicted.emplace_back(oldestUri, eit->second.fileUri);
            m_map.erase(eit);
          }
          m_fifo.pop_front();
        }
      }
      // Purge outside the lock: no disk I/O (and no eviction latency) on
      // the caller's thread; deletion is deferred by a grace period.
      DeferredFilePurger::instance().schedule(std::move(evicted));
    }

    std::optional<std::string> remove(const std::string& uri) {
      std::unique_lock lock(m_mutex);
      auto it = m_map.find(uri);
      if (it == m_map.end()) {
        return std::nullopt;
      }
      std::string fileUri = std::move(it->second.fileUri);
      m_fifo.erase(it->second.fifoPos);
      m_map.erase(it);
      return fileUri;
    }

   private:
    struct Entry {
      std::string fileUri;
      std::list<std::string>::iterator fifoPos;
    };

    std::shared_mutex m_mutex;
    std::unordered_map<std::string, Entry> m_map;
    std::list<std::string> m_fifo;
  };

  // Deferred purger for files evicted by RemoteImageFifoCache::set().
  //
  // Why deferred: eviction removes the mapping synchronously (cache state
  // stays consistent), but the physical file may still be in use -- a
  // component can be handed file://<uri> milliseconds before ArkUI's async
  // (worker-thread) image pipeline actually opens it. Deleting inline on the
  // main thread wins that race and blanks the in-flight render. Deferring the
  // delete keeps the invariant in the safe direction: "mapping gone, file
  // still there" can never blank an image (the read path re-validates
  // existence anyway), while "mapping there, file gone" would.
  //
  // Grace period: only needs to exceed the worst-case resolve -> ArkUI-open
  // latency (measured ~2ms; 10s is a generous bound covering main-thread
  // jank and image-pipeline queuing). Re-download safety does NOT rely on
  // the grace length: before deleting, the task re-checks remoteImageCache --
  // if the uri was registered again (canonical cacheKey path is identical),
  // the fresh file is kept.
  //
  // Threading: reuses the framework's ThreadTaskRunner (dedicated named
  // thread hosting a uv EventLoop -- the same primitive that backs the
  // RNOH_JS / RNOH_BACKGROUND threads) with its uv::Timer-based
  // runDelayedTask; no hand-rolled thread code. The singleton is
  // intentionally leaked: its destructor never runs at process exit,
  // avoiding a uv-loop join during static destruction (same idea as the
  // detached cleanup thread in TaskExecutor::~TaskExecutor). Pending deletes
  // lost at process death leave orphan cache files behind, which the next
  // startup's scanDiskCache simply re-indexes -- benign.
  class DeferredFilePurger {
   public:
    static DeferredFilePurger& instance() {
      static DeferredFilePurger* purger = new DeferredFilePurger();
      return *purger;
    }

    // Called from RemoteImageFifoCache::set() OUTSIDE its lock. Cheap for
    // the caller (main thread): queue lock + uv async send, no disk I/O.
    void schedule(std::vector<std::pair<std::string, std::string>> evicted) {
      if (evicted.empty()) {
        return;
      }
      m_runner.runDelayedTask(
          [evicted = std::move(evicted)] {
            for (const auto& [evictedUri, evictedFileUri] : evicted) {
              // Skip when the uri was registered again in the meantime
              // (re-downloaded onto the same canonical path) -- deleting
              // would remove the fresh file.
              if (remoteImageCache.find(evictedUri).has_value()) {
                continue;
              }
              // Best-effort delete; errors ignored (the mapping is already
              // gone, and the app cache dir is reclaimable by the OS).
              std::error_code ec;
              std::filesystem::remove(fileUriToPath(evictedFileUri), ec);
            }
          },
          GRACE_PERIOD_MS);
    }

   private:
    DeferredFilePurger() = default;

    static constexpr uint64_t GRACE_PERIOD_MS = 10 * 1000;

    ThreadTaskRunner m_runner{"RNOH_IMG_PURGE"};
  };

  // ---- Process-global cache state (shared across instances) ----
  static inline RemoteImageFifoCache remoteImageCache;
  // WORM: written once via call_once at first construction, read-only after.
  // Unlocked reads are safe: call_once completion happens-before any later
  // constructor returns, and construction happens-before task dispatch to
  // every reader (resolveImageSource is only reached via dispatched tasks).
  static inline std::unordered_map<std::string, std::string> imageDiskCacheMap;
  static inline std::once_flag s_diskCacheScanOnce;

  // ---- Per-instance state ----
  std::mutex m_uriListenersMapMutex;
  std::unordered_map<std::string, std::vector<ImageSourceUpdateListener*>>
      uriListenersMap;
  std::thread::id m_mainThreadId = std::this_thread::get_id();
  std::weak_ptr<RNInstance> m_rnInstance; // used by the video callSync path
  std::unordered_map<std::string, std::string> imageSourceByName;
  std::string m_cacheDir;

  void assertMainThread() {
    RNOH_ASSERT_MSG(
        m_mainThreadId == std::this_thread::get_id(),
        "ImageSourceResolver video trigger must run on the main thread");
  }

  void onCacheFileReady(const std::string& uri, const std::string& fileUri) {
    // Verify the file is really on disk before registering, so the cache
    // never holds an entry for a missing file (do not trust the broadcast).
    std::error_code ec;
    if (!std::filesystem::exists(fileUriToPath(fileUri), ec)) {
      return;
    }
    remoteImageCache.set(uri, fileUri);
    notifyListenersForUri(uri);
  }

  // Returns the cached file:// URI, or nullopt if the entry or its file is
  // gone.
  std::optional<std::string> findValidCachedFileUri(const std::string& uri) {
    auto fileUri = remoteImageCache.find(uri);
    if (!fileUri.has_value()) {
      return std::nullopt;
    }
    std::error_code ec;
    if (!std::filesystem::exists(fileUriToPath(fileUri.value()), ec)) {
      return std::nullopt;
    }
    return fileUri;
  }

  // Moves listeners out and erases the entry (one-shot notification: the
  // listener re-registers on its next resolveImageSource call).
  void notifyListenersForUri(const std::string& uri) {
    std::vector<ImageSourceUpdateListener*> listeners;
    {
      std::lock_guard<std::mutex> lock(m_uriListenersMapMutex);
      auto it = uriListenersMap.find(uri);
      if (it == uriListenersMap.end()) {
        return;
      }
      listeners = std::move(it->second);
      uriListenersMap.erase(it);
    }
    for (auto listener : listeners) {
      listener->onImageSourceCacheUpdate();
    }
  }

  void scanDiskCache() {
    std::error_code ec;
    if (!std::filesystem::exists(m_cacheDir, ec)) {
      if (ec && ec != std::errc::no_such_file_or_directory) {
        LOG(ERROR) << "ImageSourceResolver: failed to access cache dir '"
                   << m_cacheDir << "': " << ec.message();
      }
      return;
    }
    try {
      for (const auto& entry :
           std::filesystem::directory_iterator(m_cacheDir)) {
        if (!entry.is_regular_file()) {
          continue;
        }
        std::string filename = entry.path().filename().string();
        filename.erase(
            std::remove(filename.begin(), filename.end(), '/'), filename.end());
        // Filenames are already cacheKeys; getDiskCacheKey is idempotent.
        auto cachedKey = getDiskCacheKey(filename);
        if (cachedKey.empty()) {
          continue;
        }
        imageDiskCacheMap[cachedKey] = m_cacheDir + "/" + cachedKey;
      }
    } catch (const std::exception& e) {
      LOG(ERROR) << "Error reading directory: " << e.what();
    }
  }

  // MUST match ArkTS RemoteImageDiskCache.getCacheKey byte-for-byte.
  std::string getDiskCacheKey(const std::string& imageUri) {
    static const std::regex imageUriReg("[^a-zA-Z0-9 \\-]");
    return std::regex_replace(imageUri, imageUriReg, "");
  }

  static std::string fileUriToPath(const std::string& fileUri) {
    if (fileUri.rfind("file://", 0) == 0) {
      return fileUri.substr(7);
    }
    return fileUri;
  }
};

} // namespace rnoh
