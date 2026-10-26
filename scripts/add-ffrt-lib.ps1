$ErrorActionPreference = 'SilentlyContinue'

$ffrt = 'e:\rnoh-new\packages\react-native-harmony\oh_modules\@ppd\ffrt\libs\arm64-v8a\libffrt_cpp.so'
Write-Host ('源 libffrt_cpp.so 存在: ' + (Test-Path $ffrt))
if (Test-Path $ffrt) { Write-Host ('大小: ' + (Get-Item $ffrt).Length + ' B') }

$pkgLibs = 'e:\rnoh-new\packages\react-native-harmony\dist-libs\arm64-v8a'
Copy-Item $ffrt $pkgLibs -Force
Write-Host ('包内已补: ' + (Test-Path (Join-Path $pkgLibs 'libffrt_cpp.so')))

$harmony = 'e:\rnoh-new\examples\HelloWorld\harmony'
$targets = @(
    (Join-Path $harmony 'oh_modules\@rnoh-new\react-native-harmony\libs\arm64-v8a'),
    (Join-Path $harmony 'entry\oh_modules\@rnoh-new\react-native-harmony\libs\arm64-v8a')
)
foreach ($t in $targets) {
    Copy-Item $ffrt $t -Force
    Write-Host ('部署 ' + $t + ' -> ' + (Test-Path (Join-Path $t 'libffrt_cpp.so')))
}

Write-Host '=== 最终 libs 清单 ==='
Get-ChildItem $pkgLibs -Filter '*.so' | ForEach-Object { Write-Host ('  {0}  {1:N2} MB' -f $_.Name, ($_.Length/1MB)) }
