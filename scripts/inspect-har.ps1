Add-Type -AssemblyName System.IO.Compression.FileSystem
$har = 'e:\rnoh-new\packages\react-native-harmony\build\default\outputs\default\react_native_harmony.har'
Write-Host ('HAR 存在: ' + (Test-Path $har))
$zip = [System.IO.Compression.ZipFile]::OpenRead($har)
Write-Host ('=== 条目总数: ' + $zip.Entries.Count)
Write-Host '=== 全部条目 ==='
$zip.Entries | ForEach-Object { Write-Host ('  {0}  {1:N0} B' -f $_.FullName, $_.Length) }
$zip.Dispose()

Write-Host ''
Write-Host '=== 包 build 目录下的 so ==='
Get-ChildItem 'e:\rnoh-new\packages\react-native-harmony\build' -Recurse -Filter '*.so' -ErrorAction SilentlyContinue | ForEach-Object { Write-Host ('  {0}  {1:N2} MB' -f ($_.FullName -replace '.*react-native-harmony\\build\\',''), ($_.Length/1MB)) }
