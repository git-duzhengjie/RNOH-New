$ErrorActionPreference = 'SilentlyContinue'

$pkg     = 'e:\rnoh-new\packages\react-native-harmony'
$srcLibs = Join-Path $pkg 'build\default\intermediates\stripped_native_libs\default\arm64-v8a'
$dstLibs = Join-Path $pkg 'dist-libs\arm64-v8a'

Write-Host ('源目录: ' + $srcLibs)
Write-Host ('目标:   ' + $dstLibs)

New-Item -ItemType Directory -Path $dstLibs -Force | Out-Null
Copy-Item (Join-Path $srcLibs '*.so') $dstLibs -Force

Write-Host '=== 发布目录 dist-libs ==='
Get-ChildItem $dstLibs -Filter '*.so' | ForEach-Object { Write-Host ('  {0}  {1:N2} MB' -f $_.Name, ($_.Length/1MB)) }

$harmony = 'e:\rnoh-new\examples\HelloWorld\harmony'
$targets = @(
    (Join-Path $harmony 'oh_modules\@rnoh-new\react-native-harmony'),
    (Join-Path $harmony 'entry\oh_modules\@rnoh-new\react-native-harmony')
)
foreach ($t in $targets) {
    $d = Join-Path $t 'libs\arm64-v8a'
    New-Item -ItemType Directory -Path $d -Force | Out-Null
    Copy-Item (Join-Path $dstLibs '*.so') $d -Force
    Write-Host ('已部署: ' + $d + '  数量=' + (Get-ChildItem $d -Filter '*.so').Count)
}
Write-Host 'DONE'
