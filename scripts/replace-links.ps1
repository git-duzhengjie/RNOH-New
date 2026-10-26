$ErrorActionPreference = 'SilentlyContinue'

$harmony = 'e:\rnoh-new\examples\HelloWorld\harmony'
$src     = 'e:\rnoh-new\packages\react-native-harmony'

$targets = @(
    (Join-Path $harmony 'oh_modules\@rnoh-new\react-native-harmony'),
    (Join-Path $harmony 'entry\oh_modules\@rnoh-new\react-native-harmony')
)

foreach ($t in $targets) {
    Write-Host ('--- 处理: ' + $t)
    if (Test-Path $t) {
        $item = Get-Item $t -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            Write-Host '    是链接/联接点，用 rmdir 仅删除链接'
            & cmd /c "rmdir `"$t`""
        } else {
            Write-Host '    真实目录，直接删除'
            Remove-Item $t -Recurse -Force
        }
    }
    New-Item -ItemType Directory -Path $t -Force | Out-Null
    robocopy $src $t /E /MT:16 /NFL /NDL /NJH /NJS /NP /XD node_modules build oh_modules .hvigor | Out-Null
    Write-Host ('    复制完成，RNOHCorePackage=' + (Test-Path (Join-Path $t 'src\main\cpp\RNOHCorePackage')))
}

Write-Host '=== 清理构建缓存 ==='
Remove-Item (Join-Path $harmony 'build') -Recurse -Force
Remove-Item (Join-Path $harmony '.hvigor') -Recurse -Force
Remove-Item (Join-Path $harmony 'entry\build') -Recurse -Force
Remove-Item (Join-Path $harmony 'entry\.cxx') -Recurse -Force
Write-Host 'DONE'
