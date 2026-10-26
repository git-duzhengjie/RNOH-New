# Copyright (C) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5.0)
project(image-cache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IMAGE_CACHE_ENABLE_TRANSFORMATION
       "Enable image transformation chain (EGL/GLES/native_effect)" ON)
option(IMAGE_CACHE_ENABLE_RESOURCE
       "Enable app Resource loading via ResourceManager (ohresmgr)" ON)

set(IMAGE_CACHE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(IMAGE_CACHE_SRC ${IMAGE_CACHE_ROOT}/src)

set(IMAGE_CACHE_PRIVATE_INCLUDE_DIRS ${IMAGE_CACHE_SRC})

list(APPEND IMAGE_CACHE_PRIVATE_INCLUDE_DIRS
    ${IMAGE_CACHE_SRC}/LoadInterceptor
    ${IMAGE_CACHE_SRC}/DecodeInterceptor
    ${IMAGE_CACHE_SRC}/DownSampler
    ${IMAGE_CACHE_SRC}/MemoryCache
    ${IMAGE_CACHE_SRC}/Animation
    ${IMAGE_CACHE_SRC}/utils
    ${IMAGE_CACHE_SRC}/Provider
)

if(IMAGE_CACHE_ENABLE_TRANSFORMATION)
    list(APPEND IMAGE_CACHE_PRIVATE_INCLUDE_DIRS
        ${IMAGE_CACHE_SRC}/transformation
        ${IMAGE_CACHE_SRC}/transformation/gputransform
        ${IMAGE_CACHE_SRC}/transformation/gpufilter
    )
endif()

set(IMAGE_CACHE_SOURCES
    src/ImageCacheJobQueue.cpp
    src/ImageCacheManagerInternal.cpp
    src/MemoryCache/ImageCacheMemoryCache.cpp
    src/MemoryCache/MemCacheCallback.cpp
    src/MemoryCache/ImageCacheData.cpp
    src/ImageCacheSource.cpp
    src/ImageCacheDispatcher.cpp
    src/ImageCacheInterceptor.cpp
    src/LoadInterceptor/ImageCacheAssetLoadInterceptor.cpp
    src/LoadInterceptor/ImageCacheResourceLoader.cpp
    src/ImageCacheLoader.cpp
    src/ImageCacheLoaderInternal.cpp
    src/ImageCacheTaskInternal.cpp
    src/ImageCacheTaskWorker.cpp
    src/utils/ImageCacheBase64Util.cpp
    src/utils/ImageCacheUriUtil.cpp
    src/DecodeInterceptor/ImageCacheDecodeUtil.cpp
    src/DecodeInterceptor/ImageCacheFileTypeUtil.cpp
    src/ImageCacheOption.cpp
    src/Animation/ImageCacheAnimController.cpp
)

set(IMAGE_CACHE_LIBS
    libimage_source.so
    libpixelmap.so
    libohresmgr.so
)

if(IMAGE_CACHE_ENABLE_TRANSFORMATION)
    find_library(EGL-lib EGL)
    find_library(GLES-lib GLESv3)
    list(APPEND IMAGE_CACHE_SOURCES
        src/transformation/transformation_mask.cpp
        src/transformation/transformation_factory.cpp
        src/transformation/transformation_gaussian_blur.cpp
        src/transformation/multi_transformation.cpp
        src/transformation/gputransform/gpu_transform.cpp
        src/transformation/gputransform/ImageCacheEGL.cpp
        src/transformation/gpufilter/gpu_kuwahara_filter.cpp
    )
    list(APPEND IMAGE_CACHE_LIBS
        libnative_effect.so
        ${EGL-lib}
        ${GLES-lib}
    )
endif()

add_library(imagecache STATIC ${IMAGE_CACHE_SOURCES})

target_link_libraries(imagecache PUBLIC ${IMAGE_CACHE_LIBS} folly glog Boost::stacktrace_addr2line)

if(IMAGE_CACHE_ENABLE_TRANSFORMATION)
    target_compile_definitions(imagecache PUBLIC IMAGE_CACHE_ENABLE_TRANSFORMATION)
endif()

if(IMAGE_CACHE_ENABLE_RESOURCE)
    target_compile_definitions(imagecache PUBLIC IMAGE_CACHE_ENABLE_RESOURCE)
endif()

target_include_directories(imagecache
    PUBLIC
        ${IMAGE_CACHE_ROOT}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
    PRIVATE
        ${IMAGE_CACHE_PRIVATE_INCLUDE_DIRS}
)
