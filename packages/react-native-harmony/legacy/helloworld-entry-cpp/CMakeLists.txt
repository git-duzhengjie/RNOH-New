# RNOH-New: HelloWorld entry native bridge
#
# ArkTS code is consumed through the HAR package in oh_modules.
# Native sources are too large for HAR distribution, so the entry module
# points CMake directly at the monorepo source tree.

cmake_minimum_required(VERSION 3.13)

project(rnoh_new_entry)

# Absolute path to the react-native-harmony package in the monorepo.
set(RNH_PACKAGE_ROOT "E:/rnoh-new/packages/react-native-harmony")

set(RNH_CPP_DIR "${RNH_PACKAGE_ROOT}/src/main/cpp")

if(NOT EXISTS "${RNH_CPP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "react-native-harmony native sources not found at ${RNH_CPP_DIR}")
endif()

add_subdirectory("${RNH_CPP_DIR}" rnoh_new_core)
