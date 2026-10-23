# RNOH-New Core Package
# 内置基础组件包：View、Text、Image

cmake_minimum_required(VERSION 3.13)

# 收集所有 .cpp 源文件
file(GLOB_RECURSE RNH_CORE_PACKAGE_SRC "*.cpp")

# 创建静态库
add_library(rnh_core_package STATIC ${RNH_CORE_PACKAGE_SRC})

# 头文件搜索路径
target_include_directories(rnh_core_package PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 链接核心库
target_link_libraries(rnh_core_package PUBLIC
    rnh_new
)

# 编译定义
target_compile_definitions(rnh_core_package PRIVATE
    RNH_CORE_PACKAGE_VERSION="0.1.0"
)
