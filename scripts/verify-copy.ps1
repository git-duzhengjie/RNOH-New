$s = 'E:\myassis\client\harmony\react_native_openharmony\src\main\cpp\third-party'
$d = 'e:\rnoh-new\packages\react-native-harmony\src\main\cpp\third-party'
$names = @('fmt','double-conversion','glog','fast_float','boost','folly','rn','prebuilt')
foreach ($n in $names) {
  $a = (Get-ChildItem (Join-Path $s $n) -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count
  $b = (Get-ChildItem (Join-Path $d $n) -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count
  $st = 'DIFF'
  if ($a -eq $b) { $st = 'OK' }
  Write-Host ("{0,-20} src={1,-6} dst={2,-6} {3}" -f $n, $a, $b, $st)
}
# 逐项列出 rn 下缺失的相对路径
$srcRn = Get-ChildItem (Join-Path $s 'rn') -Recurse -File | ForEach-Object { $_.FullName.Substring((Join-Path $s 'rn').Length) }
$dstRn = Get-ChildItem (Join-Path $d 'rn') -Recurse -File | ForEach-Object { $_.FullName.Substring((Join-Path $d 'rn').Length) }
$missing = Compare-Object -ReferenceObject $srcRn -DifferenceObject $dstRn | Where-Object { $_.SideIndicator -eq '<=' }
Write-Host "=== rn 缺失文件数: $($missing.Count) ==="
$missing | Select-Object -First 20 | ForEach-Object { Write-Host $_.InputObject }
