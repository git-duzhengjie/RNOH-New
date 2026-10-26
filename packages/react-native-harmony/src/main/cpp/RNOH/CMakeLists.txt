file(GLOB_RECURSE RNOH_SRC_FILES "*.cpp")

# code in JSEngine are to be conditionally added based on USE_HERMES
list(FILTER RNOH_SRC_FILES EXCLUDE REGEX "JSEngine/.*$")
list(FILTER RNOH_SRC_FILES EXCLUDE REGEX "ImageCache/.*$")
list(FILTER RNOH_SRC_FILES EXCLUDE REGEX "tests/.*$")

option(IMAGE_CACHE_ENABLED "Enable image cache feature" OFF)

if(IMAGE_CACHE_ENABLED)
    set(RNOH_IMAGE_CACHE_BRIDGE_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/ImageCache/src/Provider/ImageCacheImageLoaderDownloader.cpp")
    add_subdirectory(ImageCache)
endif()

add_library(
  rnoh_core SHARED
  ${RNOH_SRC_FILES}
  $<$<BOOL:${IMAGE_CACHE_ENABLED}>:${RNOH_IMAGE_CACHE_BRIDGE_SRC}>)
target_include_directories(rnoh_core PUBLIC "${RNOH_CPP_DIR}" "${RNOH_APP_DIR}")
if(IMAGE_CACHE_ENABLED)
    target_include_directories(rnoh_core PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/ImageCache/include")
endif()

target_link_libraries(
  rnoh_core
  PUBLIC libace_napi.z.so
         libace_ndk.z.so
         libbundle_ndk.z.so
         libdeviceinfo_ndk.z.so
         libhiappevent_ndk.z.so
         libhilog_ndk.z.so
         librawfile.z.so
         libnative_vsync.so
         libnative_drawing.so
         libnative_display_soloist.so
         libqos.so
         $<$<BOOL:${IMAGE_CACHE_ENABLED}>:imagecache>
         Boost::context
         Boost::stacktrace_addr2line
         Boost::locale
         glog
         uv
         folly
         reactnative
         c++_shared
         )

target_compile_options(rnoh_core PUBLIC ${folly_FLAGS})
target_compile_features(rnoh_core PUBLIC cxx_std_20)

if(DEFINED LOG_VERBOSITY_LEVEL)
  message("LOG_VERBOSITY_LEVEL is set to: ${LOG_VERBOSITY_LEVEL}")
  target_compile_definitions(rnoh_core
                             PUBLIC LOG_VERBOSITY_LEVEL=${LOG_VERBOSITY_LEVEL})
endif()

if(USE_HERMES)
  add_subdirectory("${RNOH_CPP_DIR}/RNOH/JSEngine/hermes")
  target_link_libraries(rnoh_core
                        PUBLIC hermestooling)
  target_compile_definitions(rnoh_core
                             PUBLIC USE_HERMES=1)
else()
  add_subdirectory("${RNOH_CPP_DIR}/RNOH/JSEngine/jsvm")
  target_link_libraries(rnoh_core PUBLIC jsvmtooling)
  target_compile_definitions(rnoh_core
                             PUBLIC USE_HERMES=0)
endif()
if(DEFINED ADD_JSVM_SO)
    add_subdirectory("${RNOH_CPP_DIR}/RNOH/JSEngine/jsvm")
endif()

if(ALL_CONTAINERS_CLICKABLE)
  target_compile_definitions(rnoh_core
                             PUBLIC ALL_CONTAINERS_CLICKABLE)
endif()

if(IMAGE_CACHE_ENABLED)
  target_compile_definitions(rnoh_core
                             PUBLIC IMAGE_CACHE_ENABLED)
endif()
