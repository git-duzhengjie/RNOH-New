$ErrorActionPreference = 'SilentlyContinue'
$libs   = 'e:\rnoh-new\packages\react-native-harmony\libs\arm64-v8a'
$readelf = 'D:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\native\llvm\bin\llvm-readelf.exe'
$nm      = 'D:\Program Files\Huawei\DevEco Studio\sdk\default\openharmony\native\llvm\bin\llvm-nm.exe'

foreach ($n in @('libreactnative.so','librnoh_core.so','librnoh_core_package.so','libjsi.so','libhermesvm.so')) {
    $p = Join-Path $libs $n
    Write-Host ('======== ' + $n)
    if (-not (Test-Path $p)) { Write-Host '  缺失'; continue }
    $out = & $readelf -d $p 2>&1
    $out | Where-Object { $_ -match 'NEEDED|SONAME' } | ForEach-Object { Write-Host ('  ' + $_.Trim()) }
}

Write-Host ''
Write-Host '======== NAPI 注册符号 (librnoh_core.so) ========'
$syms = & $nm -D --defined-only (Join-Path $libs 'librnoh_core.so') 2>&1
$syms | Where-Object { $_ -match 'napi|Init|Register' } | Select-Object -First 15 | ForEach-Object { Write-Host ('  ' + $_.Trim()) }

Write-Host ''
Write-Host '======== bundle.harmony.js 头部 ========'
$b = 'e:\rnoh-new\examples\HelloWorld\harmony\entry\src\main\resources\rawfile\bundle.harmony.js'
Write-Host ('  存在: ' + (Test-Path $b))
if (Test-Path $b) {
    $bytes = [IO.File]::ReadAllBytes($b)
    Write-Host ('  大小: {0:N0} B' -f $bytes.Length)
    Write-Host ('  前8字节(hex): ' + (($bytes[0..7] | ForEach-Object { $_.ToString('x2') }) -join ' '))
    $head = [Text.Encoding]::ASCII.GetString($bytes[0..80])
    Write-Host ('  前80字符: ' + ($head -replace '[^\x20-\x7E]', '.'))
}
