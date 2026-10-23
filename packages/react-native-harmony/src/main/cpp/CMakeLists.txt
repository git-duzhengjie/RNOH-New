# RNOH-New: React Native for HarmonyOS
# Root CMake configuration

cmake_minimum_required(VERSION 3.13)
project(rnoh_new)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_BUILD_RPATH TRUE)

# Options
option(WITH_HITRACE "Enable HiTrace" OFF)
option(WITH_INSPECTOR "Enable Chrome Inspector" OFF)
option(BUILD_TESTS "Build tests" OFF)

# Paths
set(RNH_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(RNH_CPP_DIR ${RNH_ROOT_DIR}/RNH)
set(RNH_THIRD_PARTY_DIR ${RNH_ROOT_DIR}/third-party)
set(RNH_PATCHES_DIR ${RNH_ROOT_DIR}/patches)

# Third-party dependencies
add_subdirectory(${RNH_THIRD_PARTY_DIR})

# Core library
add_subdirectory(${RNH_CPP_DIR})

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(${RNH_CPP_DIR}/tests)
endif()
