# 获取自建运行时所需的第三方源码
#
# 用法：
#   powershell -ExecutionPolicy Bypass -File scripts/vendor-thirdparty.ps1
#   powershell -ExecutionPolicy Bypass -File scripts/vendor-thirdparty.ps1 -Source "D:\path\to\react_native_openharmony\src\main\cpp\third-party"
#
# 说明：
#   第三方源码体积约 790MB（boost 487MB + hermes API 88MB + prebuilt 162MB 等），
#   不适合纳入 git 仓库，因此通过本脚本从官方 RNOH 源码树同步。

param(
    [string]$Source = 'E:\myassis\client\harmony\react_native_openharmony\src\main\cpp\third-party',
    [string]$Target = (Join-Path $PSScriptRoot '..\packages\react-native-harmony\src\main\cpp\third-party')
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Source)) {
    Write-Error "第三方源码目录不存在: $Source`n请通过 -Source 指定官方 RNOH 的 third-party 目录。"
}

$Target = [IO.Path]::GetFullPath($Target)
Write-Host "源目录:   $Source"
Write-Host "目标目录: $Target"
New-Item -ItemType Directory -Path $Target -Force | Out-Null

# 需要完整复制的子目录
$dirs = @('fmt', 'double-conversion', 'glog', 'fast_float', 'boost', 'folly', 'rn', 'prebuilt')
foreach ($d in $dirs) {
    $s = Join-Path $Source $d
    if (-not (Test-Path $s)) { Write-Warning "跳过（源不存在）: $d"; continue }
    Write-Host "同步 $d ..."
    robocopy $s (Join-Path $Target $d) /E /MT:16 /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
}

# Hermes 只需头文件（引擎二进制走 prebuilt/libhermesvm.so）
foreach ($d in @('API', 'public')) {
    $s = Join-Path $Source "hermes\$d"
    if (-not (Test-Path $s)) { Write-Warning "跳过（源不存在）: hermes\$d"; continue }
    Write-Host "同步 hermes\$d ..."
    robocopy $s (Join-Path $Target "hermes\$d") /E /MT:8 /NFL /NDL /NJH /NJS /NP /R:1 /W:1 | Out-Null
}
Copy-Item (Join-Path $Source 'hermes\LICENSE') (Join-Path $Target 'hermes\LICENSE') -Force -ErrorAction SilentlyContinue

# 顶层 CMakeLists
Copy-Item (Join-Path $Source 'CMakeLists.txt') (Join-Path $Target 'CMakeLists.txt') -Force

Write-Host ''
Write-Host '=== 校验 ==='
$checks = @(
    'CMakeLists.txt',
    'fmt\CMakeLists.txt',
    'double-conversion\CMakeLists.txt',
    'glog\CMakeLists.txt',
    'fast_float\CMakeLists.txt',
    'boost\CMakeLists.txt',
    'folly\CMakeLists.txt',
    'rn\ReactCommon',
    'rn\ReactCommon\hermes\inspector-modern',
    'rn\ReactCommon\react\runtime\hermes\CMakeLists.txt',
    'hermes\API\jsi\jsi\jsi.h',
    'hermes\public\hermes\hermes.h',
    'prebuilt\release\arm64-v8a\libhermesvm.so'
)
$failed = 0
foreach ($c in $checks) {
    $ok = Test-Path (Join-Path $Target $c)
    if (-not $ok) { $failed++ }
    Write-Host ("  [{0}] {1}" -f $(if ($ok) { 'OK' } else { 'MISSING' }), $c)
}

if ($failed -gt 0) {
    Write-Warning "有 $failed 项校验未通过，请检查源目录是否完整。"
} else {
    Write-Host ''
    Write-Host '第三方源码同步完成，可以开始编译。'
}
Write-Host '注意：包内未包含 libevent 与 hermes 引擎源码，构建链不依赖它们。'
