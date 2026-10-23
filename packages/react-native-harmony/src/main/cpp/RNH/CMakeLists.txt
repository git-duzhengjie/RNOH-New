# RNOH-New Core C++ Library

cmake_minimum_required(VERSION 3.13)

# Collect source files
file(GLOB_RECURSE RNH_SRC_FILES "*.cpp")
list(FILTER RNH_SRC_FILES EXCLUDE REGEX "tests/.*$")

# Create shared library
add_library(rnh_new SHARED ${RNH_SRC_FILES})

# Include directories
target_include_directories(rnh_new PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RNH_THIRD_PARTY_DIR}/rn/ReactCommon
    ${RNH_PATCHES_DIR}/react_native_core
)

# Link libraries
target_link_libraries(rnh_new PUBLIC
    # ArkUI
    ace_napi
    ace_arkui

    # Third-party
    folly
    glog
    fmt
    double-conversion
    fast_float
    boost

    # React Native
    react_renderer_core
    react_renderer_graphics
    react_renderer_mounting
    react_renderer_uimanager
    react_nativemodule_core
    jsinspector

    # JS Engine
    hermes
    jsi

    # HarmonyOS
    libace_napi.z.so
    libhilog_ndk.z.so
    libnative_drawing.so
    libnative_window.so
)

# Compile definitions
target_compile_definitions(rnh_new PRIVATE
    RNH_VERSION="0.1.0"
    RNH_DEBUG=1
)

if(WITH_HITRACE)
    target_compile_definitions(rnh_new PRIVATE WITH_HITRACE=1)
    target_link_libraries(rnh_new PUBLIC libhitrace_ndk.z.so)
endif()

# Performance profiler (RNH_PERF_PROFILE)
# ON  → 启用 RAII 计时器与性能统计（有轻微运行时开销）
# OFF → 所有剖析宏展开为空，零开销（默认）
option(RNH_PERF_PROFILE "Enable RNH performance profiler" OFF)
if(RNH_PERF_PROFILE)
    target_compile_definitions(rnh_new PRIVATE RNH_PERF_PROFILE=1)
endif()
