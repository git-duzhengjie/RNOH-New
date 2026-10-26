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

#ifndef IMAGE_CACHE_CONFIG_H
#define IMAGE_CACHE_CONFIG_H

/**
 * @brief Compilation switch for the image transformation chain.
 *
 * RNOH tester enables this by default in CMake
 * (IMAGE_CACHE_ENABLE_TRANSFORMATION ON):
 * - Compiles src/transformation/ (EGL/GLES/native_effect, headers not exposed
 * externally)
 * - Externally, transformations are configured only via Option.transformSpec
 *
 * When disabled:
 *   set(IMAGE_CACHE_ENABLE_TRANSFORMATION OFF)
 *
 * RNOH Image side: derives Option.transformSpec from standard ImageProps (e.g.,
   * blurRadius), and automatically calls PrepareTransformations via
 * ImageCacheOption::Create.
 */

/**
 * @brief Compilation switch for loading app-builtin Resource IDs.
 *
 * When enabled (IMAGE_CACHE_ENABLE_RESOURCE defined):
 * - Compiles ImageCacheResourceLoadInterceptor (loads resource IDs via
 * ImageCacheSource::SetResource)
 *
 * The following capabilities are unaffected by this macro and always available:
 * - ImageCacheManager::Create (ImageCacheContext, created by RNInstanceFactory)
 * - ImageCacheUriUtil / ImageCacheResourceLoader (URI classification,
 * file/rawfile reading)
 *
 * CMake configuration:
 *   set(IMAGE_CACHE_ENABLE_RESOURCE OFF)
 *   add_subdirectory(ImageCache)
 */

constexpr size_t SINGLE_FRAME_SAFE_LIMIT = 4096 * 4096; // ~16.8M pixels
// CreatePixelmapList decodes all frames at once; cap batch frame count to avoid
// native stack/memory blow-up on high-frame-count GIF/WebP.
constexpr uint32_t MAX_BATCH_FRAME_COUNT = 64;

// Suspend timeout
// Maximum time (ms) a task may remain suspended before being aborted.
constexpr uint64_t SUSPEND_TIMEOUT_MS = 300000;

// Frame prefetch
// Number of frames to decode ahead during per-frame animation playback.
constexpr int DEFAULT_PREFETCH_AHEAD = 3;

#endif // IMAGE_CACHE_CONFIG_H
