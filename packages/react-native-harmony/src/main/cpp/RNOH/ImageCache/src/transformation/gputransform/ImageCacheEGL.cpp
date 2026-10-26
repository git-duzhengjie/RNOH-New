/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "ImageCacheEGL.h"
#include "ImageCacheLog.h"
#include <GLES3/gl3.h>

namespace ImageCachePro {

EGLDisplay ImageCacheEGL::sDisplay_ = EGL_NO_DISPLAY;
EGLConfig ImageCacheEGL::sConfig_ = EGL_NO_CONFIG_KHR;
EGLSurface ImageCacheEGL::sSurface_ = EGL_NO_SURFACE;
EGLContext ImageCacheEGL::sMainContext_ = EGL_NO_CONTEXT;
bool ImageCacheEGL::sSharedReady_ = false;
std::atomic<int> ImageCacheEGL::sRefCount_{0};
std::atomic<int> ImageCacheEGL::sActiveGpuTransforms_{0};
std::mutex ImageCacheEGL::sMutex_;

bool ImageCacheEGL::InitShared() {
  std::lock_guard<std::mutex> lock(sMutex_);
  if (sSharedReady_) {
    sRefCount_++;
    return true;
  }

  const EGLint confAttr[] = {
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES3_BIT_KHR,
      EGL_SURFACE_TYPE,
      EGL_PBUFFER_BIT,
      EGL_RED_SIZE,
      8,
      EGL_GREEN_SIZE,
      8,
      EGL_BLUE_SIZE,
      8,
      EGL_ALPHA_SIZE,
      8,
      EGL_DEPTH_SIZE,
      16,
      EGL_STENCIL_SIZE,
      8,
      EGL_NONE};
  const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  const EGLint surfaceAttr[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};

  EGLint eglMajVers;
  EGLint eglMinVers;
  EGLint numConfigs;

  sDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (sDisplay_ == EGL_NO_DISPLAY) {
    LOG(ERROR) << "EGL Get Display Failed";
    return false;
  }

  if (!eglInitialize(sDisplay_, &eglMajVers, &eglMinVers)) {
    LOG(ERROR) << "EGL Initialize Failed";
    sDisplay_ = EGL_NO_DISPLAY;
    return false;
  }

  if (!eglChooseConfig(sDisplay_, confAttr, &sConfig_, 1, &numConfigs)) {
    LOG(ERROR) << "EGL Choose Config Failed";
    eglTerminate(sDisplay_);
    sDisplay_ = EGL_NO_DISPLAY;
    return false;
  }

  sSurface_ = eglCreatePbufferSurface(sDisplay_, sConfig_, surfaceAttr);
  if (sSurface_ == EGL_NO_SURFACE) {
    LOG(ERROR) << "EGL Create Pbuffer Surface Failed";
    eglTerminate(sDisplay_);
    sDisplay_ = EGL_NO_DISPLAY;
    return false;
  }

  sMainContext_ = eglCreateContext(sDisplay_, sConfig_, EGL_NO_CONTEXT, ctxAttr);
  if (sMainContext_ == EGL_NO_CONTEXT) {
    LOG(ERROR) << "EGL Create Context Failed";
    eglDestroySurface(sDisplay_, sSurface_);
    sSurface_ = EGL_NO_SURFACE;
    eglTerminate(sDisplay_);
    sDisplay_ = EGL_NO_DISPLAY;
    return false;
  }

  if (!eglMakeCurrent(sDisplay_, sSurface_, sSurface_, sMainContext_)) {
    LOG(ERROR) << "EGL Make Current Failed";
    eglDestroyContext(sDisplay_, sMainContext_);
    sMainContext_ = EGL_NO_CONTEXT;
    eglDestroySurface(sDisplay_, sSurface_);
    sSurface_ = EGL_NO_SURFACE;
    eglTerminate(sDisplay_);
    sDisplay_ = EGL_NO_DISPLAY;
    return false;
  }

  sRefCount_ = 1;
  sSharedReady_ = true;
  return true;
}

void ImageCacheEGL::DestroyShared() {
  std::lock_guard<std::mutex> lock(sMutex_);
  if (!sSharedReady_) {
    return;
  }
  sRefCount_--;
  if (sRefCount_ > 0) {
    return;
  }
  if (sActiveGpuTransforms_.load(std::memory_order_acquire) > 0) {
    sRefCount_++;
    LOG(WARNING) << "DestroyShared deferred: GPU transforms still active";
    return;
  }
  sSharedReady_ = false;

  if (sDisplay_ != EGL_NO_DISPLAY) {
    eglMakeCurrent(
        sDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (sMainContext_ != EGL_NO_CONTEXT) {
      eglDestroyContext(sDisplay_, sMainContext_);
      sMainContext_ = EGL_NO_CONTEXT;
    }
    if (sSurface_ != EGL_NO_SURFACE) {
      eglDestroySurface(sDisplay_, sSurface_);
      sSurface_ = EGL_NO_SURFACE;
    }
    eglReleaseThread();
    eglTerminate(sDisplay_);
    sDisplay_ = EGL_NO_DISPLAY;
  }
}

bool ImageCacheEGL::IsSharedReady() {
  std::lock_guard<std::mutex> lock(sMutex_);
  return sSharedReady_;
}

bool ImageCacheEGL::BeginGpuTransform() {
  sActiveGpuTransforms_.fetch_add(1, std::memory_order_acq_rel);
  return IsSharedReady();
}

void ImageCacheEGL::EndGpuTransform() {
  sActiveGpuTransforms_.fetch_sub(1, std::memory_order_acq_rel);
}

ImageCacheEGL& ImageCacheEGL::GetInstance() {
  thread_local ImageCacheEGL instance;
  return instance;
}

bool ImageCacheEGL::EnsureInitialized() {
  if (initialized_) {
    return true;
  }
  initialized_ = InitSharedContext();
  return initialized_;
}

bool ImageCacheEGL::InitSharedContext() {
  std::lock_guard<std::mutex> lock(sMutex_);
  if (!sSharedReady_ || sDisplay_ == EGL_NO_DISPLAY ||
      sMainContext_ == EGL_NO_CONTEXT) {
    return false;
  }

  const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  sharedCtx_ = eglCreateContext(sDisplay_, sConfig_, sMainContext_, ctxAttr);
  if (sharedCtx_ == EGL_NO_CONTEXT) {
    LOG(ERROR) << "Failed to create shared EGL context";
    return false;
  }

  const EGLint surfaceAttr[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  sharedSurface_ = eglCreatePbufferSurface(sDisplay_, sConfig_, surfaceAttr);
  if (sharedSurface_ == EGL_NO_SURFACE) {
    LOG(ERROR) << "Failed to create shared PBuffer surface";
    eglDestroyContext(sDisplay_, sharedCtx_);
    sharedCtx_ = EGL_NO_CONTEXT;
    return false;
  }

  if (!eglMakeCurrent(sDisplay_, sharedSurface_, sharedSurface_, sharedCtx_)) {
    LOG(ERROR) << "Failed to make shared context current";
    eglDestroySurface(sDisplay_, sharedSurface_);
    sharedSurface_ = EGL_NO_SURFACE;
    eglDestroyContext(sDisplay_, sharedCtx_);
    sharedCtx_ = EGL_NO_CONTEXT;
    return false;
  }

  return true;
}

void ImageCacheEGL::DestroySharedContext() {
  std::lock_guard<std::mutex> lock(sMutex_);
  if (sDisplay_ == EGL_NO_DISPLAY) {
    return;
  }
  if (sharedCtx_ != EGL_NO_CONTEXT || sharedSurface_ != EGL_NO_SURFACE) {
    eglMakeCurrent(
        sDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (sharedSurface_ != EGL_NO_SURFACE) {
      eglDestroySurface(sDisplay_, sharedSurface_);
      sharedSurface_ = EGL_NO_SURFACE;
    }
    if (sharedCtx_ != EGL_NO_CONTEXT) {
      eglDestroyContext(sDisplay_, sharedCtx_);
      sharedCtx_ = EGL_NO_CONTEXT;
    }
  }
  eglReleaseThread();
}

ImageCacheEGL::~ImageCacheEGL() {
  DestroySharedContext();
}

int ImageCacheEGL::GetMaxTextureSize() const {
  GLint maxTextureSize = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
  return maxTextureSize;
}

} // namespace ImageCachePro
