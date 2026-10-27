cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0079 NEW) # Allow to link libraries from different
# directories than targets

project(rnoh)
set(RNOH_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(WITH_HITRACE_SYSTRACE "enables HiTrace-based Systrace implementation"
       OFF)
option(WITH_HITRACE_REACT_MARKER
       "enables HiTrace-based React Marker implementation" OFF)

if(NOT DEFINED ALL_CONTAINERS_CLICKABLE)
  set(ALL_CONTAINERS_CLICKABLE ON)
endif()
message("ALL_CONTAINERS_CLICKABLE is set to: ${ALL_CONTAINERS_CLICKABLE}")

if(NOT DEFINED USE_HERMES)
  set(USE_HERMES ON)
endif()

if(DEFINED PACKAGE_FIND_FILE)
  include(${PACKAGE_FIND_FILE})
endif()

# Fallback: ensure ffrt imported target exists for all ABIs (e.g. x86_64),
# where hvigor may not generate summary.cmake / set ffrt_DIR automatically.
# Create the imported target directly from the @ppd/ffrt ohpm package so it
# does not depend on hvigor-generated config files surviving a clean build.
if(NOT TARGET ffrt::ffrt_cpp)
  set(_ffrt_pkg_root
      "${RNOH_CPP_DIR}/../../../oh_modules/@ppd/ffrt")
  get_filename_component(_ffrt_pkg_root "${_ffrt_pkg_root}" ABSOLUTE)
  set(_ffrt_so "${_ffrt_pkg_root}/libs/${OHOS_ARCH}/libffrt_cpp.so")
  if(EXISTS "${_ffrt_so}")
    message(STATUS "ffrt fallback: ${_ffrt_so}")
    add_library(ffrt::ffrt_cpp SHARED IMPORTED)
    set_target_properties(ffrt::ffrt_cpp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_ffrt_pkg_root}/include"
        IMPORTED_LOCATION "${_ffrt_so}")
  else()
    message(WARNING "ffrt fallback package not found: ${_ffrt_so}")
  endif()
endif()

if(PARALLELIZATION_ENABLE)
  message("PARALLELIZATION OPTIMIZATION is enabled!")
  add_compile_definitions(PARALLELIZATION_ON)
endif()

# RNOH: 不要开启 RN_SERIALIZABLE_STATE。
# RN 0.88 中该宏还会激活大量 Android 专用代码（fbjni、toAndroidRepr 等），
# 鸿蒙侧无法编译。rawProps 需求已通过修改 vendor 的 Props.h / Props.cpp /
# ConcreteComponentDescriptor.h 解耦实现（见各文件中 RNOH 注释）。

if(SPLIT_MUTATION_ENABLE)
  message("SPLIT MUTATION is enabled!")
  add_compile_definitions(SPLIT_MUTATION_ON)
endif()

set(RNOH_FRAMEWORK_VERSION "0.84")
set(RNOH_PACKAGE_JSON_PATH "${RNOH_CPP_DIR}/../../../oh-package.json5")
set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${RNOH_PACKAGE_JSON_PATH}"
)
if(EXISTS "${RNOH_PACKAGE_JSON_PATH}")
  file(READ "${RNOH_PACKAGE_JSON_PATH}" RNOH_PACKAGE_JSON_CONTENT)
  string(
    REGEX MATCH
    "\"?version\"?[ \t\r\n]*:[ \t\r\n]*['\"]([^'\"]+)['\"]"
    RNOH_PACKAGE_JSON_VERSION_MATCH
    "${RNOH_PACKAGE_JSON_CONTENT}"
  )
  if(CMAKE_MATCH_1)
    set(RNOH_FRAMEWORK_VERSION "${CMAKE_MATCH_1}")

    string(REPLACE "." ";" VERSION_PARTS "${CMAKE_MATCH_1}")
    list(LENGTH VERSION_PARTS VERSION_PARTS_LENGTH)

    list(GET VERSION_PARTS 0 VERSION_MAJOR)
    list(GET VERSION_PARTS 1 VERSION_MINOR)

    if(VERSION_PARTS_LENGTH GREATER 2)
      list(GET VERSION_PARTS 2 VERSION_PATCH)
    else()
      set(VERSION_PATCH "0")
    endif()

    if(VERSION_PARTS_LENGTH GREATER 3)
      list(GET VERSION_PARTS 3 VERSION_BUILD)
    else()
      set(VERSION_BUILD "0")
    endif()

    set(VERSION_STRING "${CMAKE_MATCH_1}")
    add_compile_definitions(
      VERSION_MAJOR=${VERSION_MAJOR}
      VERSION_MINOR=${VERSION_MINOR}
      VERSION_PATCH=${VERSION_PATCH}
      VERSION_BUILD=${VERSION_BUILD}
      VERSION_STRING="${VERSION_STRING}"
    )
  endif()
endif()
add_compile_definitions(RNOH_FRAMEWORK_VERSION="${RNOH_FRAMEWORK_VERSION}")

if (USE_HERMES)
  set(HERMES_V1_ENABLED ON)
endif()

if(${CMAKE_BUILD_TYPE} MATCHES Debug OR REACT_NATIVE_DEBUG_OPTIMIZED)
  if(NOT DEFINED HERMES_ENABLE_DEBUGGER)
    set(HERMES_ENABLE_DEBUGGER ON)
  endif()
endif()
message("USE_HERMES is set to: ${USE_HERMES}")
message("HERMES_V1_ENABLED is set to: ${HERMES_V1_ENABLED}")
message("HERMES_ENABLE_DEBUGGER is set to: ${HERMES_ENABLE_DEBUGGER}")
message("REACT_NATIVE_DEBUG_OPTIMIZED is set to: ${REACT_NATIVE_DEBUG_OPTIMIZED}")

set(CMAKE_SKIP_BUILD_RPATH TRUE)

add_compile_options("-Wno-error=unused-command-line-argument")

add_compile_options(
    "-fstack-protector-strong"
    "-fPIC"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options(
        "-Wl,-z,relro"
        "-Wl,-z,now"
        "-Wl,-z,noexecstack"
        "-pie"
        "-Wl,--build-id"
    )
    message(STATUS "Release build: Symbols will be separated for debugging")
else()
    add_link_options(
        "-Wl,-z,relro"
        "-Wl,-z,now"
        "-Wl,-z,noexecstack"
        "-pie"
    )
    message(STATUS "Debug build: Symbols retained for debugging")
endif()

# @internal
option(ENABLE_COMPILE_OPTIMIZATIONS "Enable or disable compiler
optimizations" ON)
set(PGO_MODE "USE" CACHE STRING "PGO mode: COLLECT( profile data for
optimization) or USE( the collected profile to optimize build)")
set(PROFDATA_FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../resources/rawfile/rnoh.profdata")
set(PROFRAW_GENERATING_PATH "/data/storage/el2/base/files/rnohprofraw")

# @actor RNOH_APP, RNOH_LIBRARY
add_library(rnoh_optimization_options INTERFACE)

if(ENABLE_COMPILE_OPTIMIZATIONS)
    # inline threshold's optimization result is code sensitive, thus not
    # included in the interface meant for developers
    add_compile_options(
        "SHELL:-mllvm -inline-threshold=225"
        "SHELL:-mllvm -inline-cold-callsite-threshold=45"
        "SHELL:-mllvm -inlinecold-threshold=45"
    )

    set(compile_optimization_option
        "-flto"
        "-fwhole-program-vtables"
        # 'SHELL:' is necessary for adding options with '-mllvm'
        "SHELL:-mllvm --enable-partial-inlining"
        "SHELL:-mllvm --tail-dup-placement"
    )
    set(link_optimization_option
        "-flto"
        "-Wl,-mllvm,--wholeprogramdevirt-check=fallback"
        "-Wl,-Bsymbolic-functions"
        "-Wl,-mllvm,--enable-partial-inlining"
        "-Wl,-mllvm,--tail-dup-placement"
    )

    include(CheckLinkerFlag)
    check_linker_flag("CXX" "-Wl,--aarch64-inline-plt" USING_BISHENG_COMPILER)
    if(OHOS_ARCH STREQUAL "arm64-v8a")
        # skip pgo related options if it's already configured
        get_directory_property(COMPILE_OPTIONS COMPILE_OPTIONS)
        string(FIND "${COMPILE_OPTIONS}" "-fprofile-generate" PROFILE_GENERATE_FOUND)
        string(FIND "${COMPILE_OPTIONS}" "-fprofile-use" PROFILE_USE_FOUND)
        if(PROFILE_GENERATE_FOUND EQUAL -1 AND PROFILE_USE_FOUND EQUAL -1)
            if(PGO_MODE STREQUAL "USE")
                add_compile_options(
                    "-fprofile-use=${PROFDATA_FILE_PATH}"
                    "-Wno-error=backend-plugin"
                )
            elseif(PGO_MODE STREQUAL "COLLECT")
                add_compile_options(
                    "-fprofile-generate=${PROFRAW_GENERATING_PATH}"
                    "SHELL:-mllvm -pgo-instrument-entry=true"
                )
                add_link_options(
                    "-fprofile-generate=${PROFRAW_GENERATING_PATH}"
                    "-Wl,-mllvm,-pgo-instrument-entry=true"
                )
            endif()
        endif()
        if(USING_BISHENG_COMPILER)
          add_link_options("-Wl,--aarch64-inline-plt")
        endif()
    endif()

    add_compile_options(${compile_optimization_option})
    add_link_options(${link_optimization_option})

    target_compile_options(rnoh_optimization_options INTERFACE ${compile_optimization_option})
    target_link_options(rnoh_optimization_options INTERFACE ${link_optimization_option})
endif()

set(NATIVERENDER_ROOT_PATH "${RNOH_CPP_DIR}")
set(third_party_dir "${RNOH_CPP_DIR}/third-party")
set(patches_dir "${RNOH_CPP_DIR}/patches")

include_directories(${NATIVERENDER_ROOT_PATH} ${NATIVERENDER_ROOT_PATH}/include)

add_subdirectory("third-party")
add_subdirectory("RNOH")
add_subdirectory("RNOHCorePackage")

add_library(rnoh INTERFACE)
target_link_libraries(rnoh INTERFACE rnoh_core rnoh_core_package rnoh_optimization_options)

# 使用 add_custom_target 实现跨目录符号分离
message(STATUS "=== Debug Symbol Separation ===")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_OBJCOPY: ${CMAKE_OBJCOPY}")
function(separate_debug_symbols)
    # 只在 Release 或 RelWithDebInfo 模式执行
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        return()
    endif()

    # 检查 CMAKE_OBJCOPY 是否存在
    if(NOT CMAKE_OBJCOPY)
        message(WARNING "CMAKE_OBJCOPY not found, skip debug symbol separation")
        return()
    endif()

    # 创建一个全局的符号分离目标
    set(symbol_target "separate_all_debug_symbols")
    if(NOT TARGET ${symbol_target})
        add_custom_target(${symbol_target} ALL
            COMMENT "Start separating all debug symbols..."
        )
    endif()

    # 遍历所有传入的目标
    foreach(target ${ARGN})
        if(NOT TARGET ${target})
            message(WARNING "${target} not found, skip")
            continue()
        endif()

        # 获取目标文件路径
        set(target_file "$<TARGET_FILE:${target}>")
        set(sym_file "${target_file}.sym")

        # 为每个目标添加自定义命令，依赖于原目标
        add_custom_command(TARGET ${symbol_target} POST_BUILD
            # 1. 导出符号
            COMMAND ${CMAKE_OBJCOPY} --only-keep-debug
                "${target_file}"
                "${sym_file}"
                || true
            COMMENT "Export debug symbols to: ${target}.sym"

            # 2. 剥离符号
            COMMAND ${CMAKE_OBJCOPY} --strip-debug --strip-unneeded
                "${target_file}"
                || true
            COMMENT "Strip debug symbols from file: ${target}.so"

            # 3. 关联符号
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink="${sym_file}"
                "${target_file}"
                || true
            COMMENT "Add debug symbol link to: ${target}.so, debug symbol file is: ${target}.so.sym"
        )

        # 确保符号分离在原目标编译完成后执行
        add_dependencies(${symbol_target} ${target})
    endforeach()
endfunction()

# 调用符号分离（确认目标名正确）
separate_debug_symbols(
    rnoh_core
    rnoh_core_package
    reactnative
    jsvmtooling
    hermestooling
    jsi
)
