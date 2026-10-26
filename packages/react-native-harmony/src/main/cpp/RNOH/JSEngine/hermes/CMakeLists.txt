
message("HERMES_V1_ENABLED is set to: ${HERMES_V1_ENABLED}")
message("HERMES_ENABLE_DEBUGGER is set to: ${HERMES_ENABLE_DEBUGGER}")

# HERMES
add_library(hermes-engine::hermesvm SHARED IMPORTED)
if(HERMES_ENABLE_DEBUGGER)
  target_compile_definitions(hermes-engine::hermesvm INTERFACE
                              HERMES_ENABLE_DEBUGGER)
  set_property(
    TARGET hermes-engine::hermesvm
    PROPERTY IMPORTED_LOCATION
           "${third_party_dir}/prebuilt/debug/${OHOS_ARCH}/libhermesvm.so")
else()
  set_property(
    TARGET hermes-engine::hermesvm
    PROPERTY IMPORTED_LOCATION
        "${third_party_dir}/prebuilt/release/${OHOS_ARCH}/libhermesvm.so")
endif()

set(hermes_include_dirs "${third_party_dir}/hermes/API"
                        "${third_party_dir}/hermes/public")

add_react_common_subdir(hermes/executor)
target_include_directories(hermes_executor_common
                           PUBLIC
                           "${hermes_include_dirs}"
                           "${patches_dir}/react_native_core")
add_react_common_subdir(react/runtime/hermes)
target_include_directories(bridgelesshermes PUBLIC "${hermes_include_dirs}")

# HERMES INSPECTOR
add_react_common_subdir(hermes/inspector-modern)
target_include_directories(hermes_inspector_modern
                           PRIVATE "${hermes_include_dirs}")
target_compile_options(hermes_inspector_modern PRIVATE ${folly_FLAGS})

if(HERMES_V1_ENABLED)
  target_compile_definitions(hermes_executor_common PRIVATE HERMES_V1_ENABLED=1)
  target_compile_definitions(bridgelesshermes PRIVATE HERMES_V1_ENABLED=1)
  target_compile_definitions(hermes_inspector_modern PRIVATE HERMES_V1_ENABLED=1)
endif()

# hermestooling is a shared library where we merge all the hermes* related libraries.
add_library(hermestooling
        SHARED
        HermesMemoryWaterMark.cpp
        $<TARGET_OBJECTS:bridgelesshermes>
        $<TARGET_OBJECTS:hermes_executor_common>
        $<TARGET_OBJECTS:hermes_inspector_modern>
)
target_link_libraries(hermestooling
        PUBLIC
        reactnative
        jsi
        hermes-engine::hermesvm
        libhilog_ndk.z.so
        libdeviceinfo_ndk.z.so
        dl
)
target_include_directories(hermestooling
        PUBLIC
        $<TARGET_PROPERTY:bridgelesshermes,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:hermes_executor_common,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:hermes_inspector_modern,INTERFACE_INCLUDE_DIRECTORIES>
)
