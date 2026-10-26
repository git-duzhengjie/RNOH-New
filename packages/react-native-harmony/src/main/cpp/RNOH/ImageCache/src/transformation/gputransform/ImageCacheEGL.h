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

#ifndef IMAGE_KNIFE_C_KEKE_IMAGEKNIFE_EGL_H
#define IMAGE_KNIFE_C_KEKE_IMAGEKNIFE_EGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <atomic>
#include <mutex>
#include <thread>
#include "ImageCacheLog.h"

namespace ImageCachePro {
class ImageCacheEGL {
 public:
  static bool InitShared();
  static void DestroyShared();
  static bool IsSharedReady();
  static bool BeginGpuTransform();
  static void EndGpuTransform();
  static ImageCacheEGL& GetInstance();
  bool EnsureInitialized();
  int GetMaxTextureSize() const;

 private:
  static EGLDisplay sDisplay_;
  static EGLConfig sConfig_;
  static EGLSurface sSurface_;
  static EGLContext sMainContext_;
  static bool sSharedReady_;
  static std::atomic<int> sRefCount_;
  static std::atomic<int> sActiveGpuTransforms_;
  static std::mutex sMutex_;

  EGLContext sharedCtx_ = EGL_NO_CONTEXT;
  EGLSurface sharedSurface_ = EGL_NO_SURFACE;
  bool initialized_ = false;

  bool InitSharedContext();
  void DestroySharedContext();

  ~ImageCacheEGL();
};
} // namespace ImageCachePro

#endif // IMAGE_KNIFE_C_KEKE_IMAGEKNIFE_EGL_H
