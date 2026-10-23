# Third-party dependencies for RNOH-New

cmake_minimum_required(VERSION 3.13)

set(third_party_dir ${CMAKE_CURRENT_SOURCE_DIR})

# Fmt
add_subdirectory("${third_party_dir}/fmt")

# Double Conversion
add_subdirectory("${third_party_dir}/double-conversion")

# Glog
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${third_party_dir}/glog")

# Fast Float
add_subdirectory("${third_party_dir}/fast_float")

# Boost (header-only subset)
set(BOOST_INCLUDE_LIBRARIES
    align
    context
    algorithm
    intrusive
    stacktrace
    locale
)
add_subdirectory("${third_party_dir}/boost" EXCLUDE_FROM_ALL)

# Folly
set(folly_include_dir "${third_party_dir}/folly")
add_subdirectory("${third_party_dir}/folly")

# Hermes (JS Engine)
set(HERMES_ENABLE_DEBUGGER OFF CACHE BOOL "" FORCE)
set(HERMES_ENABLE_INTL OFF CACHE BOOL "" FORCE)
set(HERMES_ENABLE_IR_NATIVE_PROFILER OFF CACHE BOOL "" FORCE)
add_subdirectory("${third_party_dir}/hermes")

# React Native ReactCommon
add_subdirectory("${third_party_dir}/rn")
