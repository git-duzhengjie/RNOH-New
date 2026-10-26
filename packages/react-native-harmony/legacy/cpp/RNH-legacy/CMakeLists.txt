# RNOH-New Core C++ Library
#
# Minimal CMake that builds only the RNOH-New native code.
# The current implementation does not depend on React Native C++ sources,
# Folly, Hermes, or Yoga; it only uses HarmonyOS system NDK libraries.

cmake_minimum_required(VERSION 3.13)

# Collect source files
file(GLOB_RECURSE RNH_SRC_FILES "*.cpp")
list(FILTER RNH_SRC_FILES EXCLUDE REGEX "tests/.*$")

# Create shared library
add_library(rnh SHARED ${RNH_SRC_FILES})

# Include directories
target_include_directories(rnh PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link HarmonyOS NDK libraries
target_link_libraries(rnh PUBLIC
    # ArkUI / NAPI
    libace_napi.z.so
    libace_ndk.z.so
    libhilog_ndk.z.so
    libnative_drawing.so
    libnative_window.so
)

# Compile definitions
target_compile_definitions(rnh PRIVATE
    RNH_VERSION="0.1.0"
    RNH_DEBUG=1
)

option(RNH_PERF_PROFILE "Enable RNH performance profiler" OFF)
if(RNH_PERF_PROFILE)
    target_compile_definitions(rnh PRIVATE RNH_PERF_PROFILE=1)
endif()
