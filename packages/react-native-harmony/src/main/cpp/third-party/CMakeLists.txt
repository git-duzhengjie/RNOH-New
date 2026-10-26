# Fmt
add_subdirectory("${third_party_dir}/fmt")

# Double Conversion 
add_subdirectory("${third_party_dir}/double-conversion")

# Glog
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${third_party_dir}/glog")

# Fast Float
add_subdirectory("${third_party_dir}/fast_float")

# Boost
set(BOOST_INCLUDE_LIBRARIES 
  align
  context
  algorithm
  intrusive
  stacktrace
  locale
)
add_subdirectory("${third_party_dir}/boost" EXCLUDE_FROM_ALL)

# FOLLY
set(folly_include_dir "${third_party_dir}/folly")
set(folly_src_dir "${third_party_dir}/folly/folly")
add_library(folly)
target_include_directories(folly PUBLIC "${folly_include_dir}")
set(folly_FLAGS
    -DFOLLY_NO_CONFIG=1
    -DFOLLY_MOBILE=1
    -DFOLLY_USE_LIBCPP=1
    -DFOLLY_HAVE_RECVMMSG=1
    -DFOLLY_HAVE_PTHREAD=1
    -std=c++20
    -Wno-comma
    -Wno-shorten-64-to-32
    -Wno-documentation
    -faligned-new)
target_compile_options(folly PUBLIC ${folly_FLAGS})

# react-native/ReactAndroid/src/main/jni/third-party/folly/CMakeLists.txt
target_sources(
  folly
  PRIVATE   "${folly_src_dir}/Conv.cpp"
#            "${folly_src_dir}/Demangle.cpp"
            "${folly_src_dir}/FileUtil.cpp"
            "${folly_src_dir}/Format.cpp"
            "${folly_src_dir}/ScopeGuard.cpp"
            "${folly_src_dir}/SharedMutex.cpp"
            "${folly_src_dir}/String.cpp"
            "${folly_src_dir}/Unicode.cpp"
            "${folly_src_dir}/concurrency/CacheLocality.cpp"
            "${folly_src_dir}/container/detail/F14Table.cpp"
            "${folly_src_dir}/detail/FileUtilDetail.cpp"
            "${folly_src_dir}/detail/Futex.cpp"
            "${folly_src_dir}/detail/SplitStringSimd.cpp"
            "${folly_src_dir}/detail/UniqueInstance.cpp"
            "${folly_src_dir}/hash/SpookyHashV2.cpp"
            "${folly_src_dir}/json/dynamic.cpp"
            "${folly_src_dir}/json/json_pointer.cpp"
            "${folly_src_dir}/json/json.cpp"
            "${folly_src_dir}/lang/CString.cpp"
            "${folly_src_dir}/lang/Exception.cpp"
            "${folly_src_dir}/lang/SafeAssert.cpp"
            "${folly_src_dir}/lang/ToAscii.cpp"
            "${folly_src_dir}/memory/detail/MallocImpl.cpp"
            "${folly_src_dir}/net/NetOps.cpp"
            "${folly_src_dir}/portability/SysUio.cpp"
            "${folly_src_dir}/synchronization/SanitizeThread.cpp"
            "${folly_src_dir}/synchronization/ParkingLot.cpp"
            "${folly_src_dir}/system/AtFork.cpp"
            "${folly_src_dir}/system/ThreadId.cpp"
 )

target_link_libraries(folly PUBLIC Boost::context Boost::algorithm
                                   fmt::fmt glog double-conversion fast_float)

add_subdirectory("rn")
