# RNOH: HarmonyOS 适配补丁
#
# RN 0.88 的 ReactCommon/jsinspector-modern/cdp/CMakeLists.txt 会执行：
#   include(${REACT_ANDROID_DIR}/src/main/jni/first-party/jni-lib-merge/SoMerging-utils.cmake)
#   target_merge_so(jsinspector_cdp)
#
# 该机制来自 Android：把多个 JNI 动态库合并成一个 so。
# HarmonyOS 上不存在 JNI 多 so 合并需求，因此这里提供 no-op 实现，
# 仅保证 CMake 配置阶段能正常通过。

function(target_merge_so target_name)
  # no-op：不做 so 合并
endfunction()
