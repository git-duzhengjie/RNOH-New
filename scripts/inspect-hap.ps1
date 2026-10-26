Add-Type -AssemblyName System.IO.Compression.FileSystem
$hap = 'E:\rnoh-new\examples\HelloWorld\harmony\entry\build\default\outputs\default\entry-default-unsigned.hap'
$zip = [System.IO.Compression.ZipFile]::OpenRead($hap)
Write-Host ('=== HAP 条目总数: ' + $zip.Entries.Count)
Write-Host '=== .so ==='
$zip.Entries | Where-Object { $_.FullName -like '*.so' } | ForEach-Object { Write-Host ('  {0}  {1:N2} MB' -f $_.FullName, ($_.Length/1MB)) }
Write-Host '=== bundle / assets ==='
$zip.Entries | Where-Object { $_.FullName -like '*bundle*' -or $_.FullName -like 'assets/*' } | Select-Object -First 15 | ForEach-Object { Write-Host ('  {0}  {1:N0} B' -f $_.FullName, $_.Length) }
Write-Host '=== libs 目录相关 ==='
$zip.Entries | Where-Object { $_.FullName -like 'libs/*' } | Select-Object -First 15 | ForEach-Object { Write-Host ('  ' + $_.FullName) }
$zip.Dispose()
