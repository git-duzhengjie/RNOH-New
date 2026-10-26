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

#ifndef IMAGE_CACHE_RESOURCE_LOADER_H
#define IMAGE_CACHE_RESOURCE_LOADER_H

#include <resourcemanager/ohresmgr.h>
#include <memory>
#include <string>
#include <vector>
#include "ImageCacheTask.h"

namespace rnoh {
class TaskExecutor;
}

namespace ImageCachePro {

class ImageCacheResourceLoader {
 public:
  static bool LoadFromFilePath(
      const std::string& path,
      std::shared_ptr<ImageCacheTask> task);

  static bool LoadFromRawfile(
      const std::string& relativePath,
      std::shared_ptr<ImageCacheTask> task,
      const NativeResourceManager* resourceManager,
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor,
      bool echoError = true);

  // Try multiple rawfile candidate paths in order; stops at first success.
  // On WORKER thread, all candidates are tried in a single MAIN-thread hop
  // to avoid per-candidate Suspend/Resume overhead.
  static bool LoadFromRawfileCandidates(
      std::vector<std::string> candidates,
      std::shared_ptr<ImageCacheTask> task,
      const NativeResourceManager* resourceManager,
      const std::shared_ptr<rnoh::TaskExecutor>& taskExecutor);
};

} // namespace ImageCachePro

#endif // IMAGE_CACHE_RESOURCE_LOADER_H
