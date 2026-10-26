/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ImageCacheAssetLoadInterceptor.h"
#include <cstring>
#include <vector>
#include "ImageCacheLog.h"
#include "ImageCacheManagerInternal.h"
#include "ImageCacheResourceLoader.h"
#include "ImageCacheTaskInternal.h"
#include "ImageCacheUriUtil.h"

namespace ImageCachePro {

namespace {

constexpr const char* ASSETS_DEST = "assets/";
constexpr const char* ASSET_PREFIX = "asset://";
constexpr const char* RAWFILE_PREFIX = "resource://RAWFILE/assets/";

std::string GetAssetsPrefixFromTask(
    const std::shared_ptr<ImageCacheTask>& task) {
  auto taskInternal = std::dynamic_pointer_cast<ImageCacheTaskInternal>(task);
  if (taskInternal == nullptr) {
    return RAWFILE_PREFIX;
  }

  auto option = taskInternal->GetOption();
  if (option == nullptr) {
    return RAWFILE_PREFIX;
  }
  if (option->GetAssetsPrefix().empty()) {
    return RAWFILE_PREFIX;
  }
  return option->GetAssetsPrefix();
}

std::string BuildResolvedAssetUri(
    const std::string& assetUri,
    const std::shared_ptr<ImageCacheTask>& task) {
  if (assetUri.rfind(ASSET_PREFIX, 0) != 0) {
    return assetUri;
  }
  return GetAssetsPrefixFromTask(task) +
      assetUri.substr(std::strlen(ASSET_PREFIX));
}

std::string GetRawfileRelativePath(const std::string& rawfileUri) {
  if (rawfileUri.rfind(RAWFILE_PREFIX, 0) != 0) {
    return std::string();
  }
  return rawfileUri.substr(std::strlen(RAWFILE_PREFIX));
}

std::string StripUrlQuery(const std::string& uri) {
  const auto queryPos = uri.find('?');
  return queryPos == std::string::npos ? uri : uri.substr(0, queryPos);
}

std::string GetUrlPathAfterHost(const std::string& uri) {
  const auto schemePos = uri.find("://");
  if (schemePos == std::string::npos) {
    return std::string();
  }
  const auto pathStart = uri.find('/', schemePos + 3);
  if (pathStart == std::string::npos || pathStart + 1 >= uri.size()) {
    return std::string();
  }
  return uri.substr(pathStart + 1);
}

std::string StripLeadingAssetsDest(const std::string& path) {
  std::string result = path;
  while (result.rfind(ASSETS_DEST, 0) == 0) {
    result = result.substr(std::strlen(ASSETS_DEST));
  }
  return result;
}

void AppendUniqueCandidate(
    std::vector<std::string>& candidates,
    const std::string& path) {
  if (path.empty()) {
    return;
  }
  for (const auto& existing : candidates) {
    if (existing == path) {
      return;
    }
  }
  candidates.push_back(path);
}

std::vector<std::string> CollectReleaseRawfileCandidates(
    const std::string& relativePath) {
  std::vector<std::string> candidates;
  if (relativePath.empty()) {
    return candidates;
  }

  const std::string bare = StripLeadingAssetsDest(relativePath);
  AppendUniqueCandidate(candidates, std::string(ASSETS_DEST) + bare);
  AppendUniqueCandidate(
      candidates, std::string(ASSETS_DEST) + ASSETS_DEST + bare);
  AppendUniqueCandidate(candidates, relativePath);
  AppendUniqueCandidate(candidates, bare);
  return candidates;
}

std::vector<std::string> CollectMetroRawfileCandidates(
    const std::string& relativePath) {
  std::vector<std::string> candidates;
  if (relativePath.empty()) {
    return candidates;
  }

  const std::string bare = StripLeadingAssetsDest(relativePath);
  AppendUniqueCandidate(candidates, relativePath);
  AppendUniqueCandidate(
      candidates, std::string(ASSETS_DEST) + ASSETS_DEST + bare);
  AppendUniqueCandidate(candidates, std::string(ASSETS_DEST) + bare);
  AppendUniqueCandidate(candidates, bare);
  return candidates;
}

bool TryLoadFromRawfileCandidates(
    std::vector<std::string> candidates,
    const std::shared_ptr<ImageCacheTask>& task,
    ImageCacheManagerInternal& manager) {
  if (candidates.empty()) {
    if (task != nullptr) {
      TASK_SET_ERROR(task, ImageCacheErrorCode::EMPTY_RAWFILE_PATH);
    }
    return false;
  }

  return ImageCacheResourceLoader::LoadFromRawfileCandidates(
      std::move(candidates),
      task,
      manager.GetNativeResourceManager(),
      manager.GetTaskExecutor());
}

bool IsMetroDevAssetUrl(const std::string& uri) {
  if (!ImageCacheUriUtil::IsRemoteUri(uri)) {
    return false;
  }
  const auto path = GetUrlPathAfterHost(StripUrlQuery(uri));
  return !path.empty() && path.find("assets/") == 0;
}

bool TryLoadReleaseRawfile(
    const std::shared_ptr<ImageCacheTask>& task,
    const std::string& relativePath,
    ImageCacheManagerInternal& manager) {
  return TryLoadFromRawfileCandidates(
      CollectReleaseRawfileCandidates(relativePath), task, manager);
}

bool ResolveAssetUri(
    const std::string& url,
    const std::shared_ptr<ImageCacheTask>& task,
    ImageCacheManagerInternal& manager) {
  const std::string assetRelative = url.substr(std::strlen("asset://"));

  // require() resources are in HAP rawfile; try rawfile first, fallback to
  // option.assetsPrefix parsed file path.
  if (TryLoadReleaseRawfile(task, assetRelative, manager)) {
    return true;
  }

  const std::string resolved = BuildResolvedAssetUri(url, task);
  const auto kind = ImageCacheUriUtil::ClassifyUri(resolved);

  if (kind == ImageCacheRnohSourceKind::File ||
      kind == ImageCacheRnohSourceKind::BundleStorage) {
    return ImageCacheResourceLoader::LoadFromFilePath(resolved, task);
  }
  if (kind == ImageCacheRnohSourceKind::Rawfile) {
    const std::string relativePath = GetRawfileRelativePath(resolved);
    return TryLoadReleaseRawfile(task, relativePath, manager);
  }
  return false;
}

bool ResolveRawfileUri(
    const std::string& url,
    const std::shared_ptr<ImageCacheTask>& task,
    ImageCacheManagerInternal& manager) {
  const std::string relativePath = GetRawfileRelativePath(url);
  return TryLoadReleaseRawfile(task, relativePath, manager);
}

bool ResolveMetroDevAsset(
    const std::string& url,
    const std::shared_ptr<ImageCacheTask>& task,
    ImageCacheManagerInternal& manager) {
  const auto path = GetUrlPathAfterHost(StripUrlQuery(url));
  return TryLoadFromRawfileCandidates(
      CollectMetroRawfileCandidates(path), task, manager);
}

} // namespace

ImageCacheAssetLoadInterceptor::ImageCacheAssetLoadInterceptor(
    ImageCacheManagerInternal& manager)
    : ImageCacheLoadInterceptor(manager) {
  name = "AssetLoadInterceptor";
  isLoadFromRemote = false;
}

bool ImageCacheAssetLoadInterceptor::Resolve(
    const std::shared_ptr<ImageCacheTask>& task) {
  std::string url;
  if (!task->GetImageSource().GetString(url)) {
    return false;
  }

  if (IsMetroDevAssetUrl(url)) {
    return ResolveMetroDevAsset(url, task, manager_);
  }

  const auto kind = ImageCacheUriUtil::ClassifyUri(url);
  if (kind == ImageCacheRnohSourceKind::Asset) {
    return ResolveAssetUri(url, task, manager_);
  }
  if (kind == ImageCacheRnohSourceKind::Rawfile) {
    return ResolveRawfileUri(url, task, manager_);
  }
  return false;
}

} // namespace ImageCachePro
