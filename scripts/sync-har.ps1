$ErrorActionPreference = 'SilentlyContinue'

$harmony = 'e:\rnoh-new\examples\HelloWorld\harmony'
$src     = 'e:\rnoh-new\packages\react-native-harmony'
$dstRoot = Join-Path $harmony 'oh_modules\@rnoh-new'
$dst     = Join-Path $dstRoot 'react-native-harmony'
$cache   = Join-Path $harmony 'oh_modules\.ohpm'

Write-Host '=== 1. 清理旧缓存 ==='
Get-ChildItem $cache -Directory | Where-Object { $_.Name -like '@rnoh-new*' } | ForEach-Object {
    Write-Host ('  remove cache: ' + $_.Name)
    Remove-Item $_.FullName -Recurse -Force
}
Remove-Item $dstRoot -Recurse -Force
Remove-Item (Join-Path $harmony 'oh-package-lock.json5') -Force
Remove-Item (Join-Path $harmony 'build') -Recurse -Force
Remove-Item (Join-Path $harmony '.hvigor') -Recurse -Force
Remove-Item (Join-Path $harmony 'entry\build') -Recurse -Force

Write-Host '=== 2. 复制最新包内容 ==='
New-Item -ItemType Directory -Path $dst -Force | Out-Null
robocopy $src $dst /E /MT:16 /NFL /NDL /NJH /NJS /NP /XD node_modules build oh_modules .hvigor | Out-Null

Write-Host '=== 3. 校验 ==='
$cpp = Join-Path $dst 'src\main\cpp'
Write-Host ('RNOHCorePackage : ' + (Test-Path (Join-Path $cpp 'RNOHCorePackage')))
Write-Host ('RNOH            : ' + (Test-Path (Join-Path $cpp 'RNOH')))
Write-Host ('third-party/boost: ' + (Test-Path (Join-Path $cpp 'third-party\boost')))
Write-Host ('prebuilt so     : ' + (Test-Path (Join-Path $cpp 'third-party\prebuilt\release\arm64-v8a\libhermesvm.so')))
Write-Host ('profdata        : ' + (Test-Path (Join-Path $dst 'src\main\resources\rawfile\rnoh.profdata')))
Write-Host ('index.ets       : ' + (Test-Path (Join-Path $dst 'index.ets')))

Write-Host '=== 4. 顶层内容 ==='
Get-ChildItem $dst -Name
