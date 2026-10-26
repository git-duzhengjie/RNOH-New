# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

cmake_minimum_required(VERSION 3.13)
set(CMAKE_VERBOSE_MAKEFILE on)

file(GLOB react_codegen_SRCS CONFIGURE_DEPENDS *.cpp react/renderer/components/FBReactNativeSpec/*.cpp)

add_library(
  react_codegen_rncore
  OBJECT
  ${react_codegen_SRCS}
)

target_include_directories(react_codegen_rncore PUBLIC . react/renderer/components/FBReactNativeSpec)

target_link_libraries(
  react_codegen_rncore
  fbjni
  jsi
  # We need to link different libraries based on whether we are building rncore or not, that's necessary
  # because we want to break a circular dependency between react_codegen_rncore and reactnative
  folly_runtime glog react_debug react_nativemodule_core react_renderer_componentregistry react_renderer_core react_renderer_debug react_renderer_graphics react_renderer_imagemanager react_renderer_mapbuffer react_utils rrc_image rrc_view turbomodulejsijni yoga
)

target_compile_reactnative_options(react_codegen_rncore PRIVATE)
