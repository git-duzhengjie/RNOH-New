cmake_minimum_required(VERSION 3.13)
set(CMAKE_VERBOSE_MAKEFILE on)

add_compile_options(-std=c++20)

file(GLOB_RECURSE bridgeless_jsvm_SRC CONFIGURE_DEPENDS *.cpp)
add_library(
        bridgelessjsvm
        OBJECT
        ${bridgeless_jsvm_SRC}
)
target_include_directories(bridgelessjsvm PUBLIC .)

target_link_libraries(bridgelessjsvm
        jsi
        jsinspector
        reactnative
        libjsvm.so
        Boost::context
        Boost::stacktrace_addr2line
        Boost::locale
)

add_library(jsvmtooling
        SHARED
        $<TARGET_OBJECTS:bridgelessjsvm>
)
target_link_libraries(jsvmtooling
        PUBLIC
        reactnative
        jsi
        libjsvm.so
)
target_include_directories(jsvmtooling
        PUBLIC
        $<TARGET_PROPERTY:bridgelessjsvm,INTERFACE_INCLUDE_DIRECTORIES>
)