param([Parameter(Mandatory=$true)][string]$EditorData)
$ErrorActionPreference = 'Stop'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = $Utf8NoBom
$OutputEncoding = $Utf8NoBom
$compiler = Join-Path $EditorData 'DotNetSdkRoslyn/csc.dll'
$runtime = Join-Path $EditorData 'NetCoreRuntime/dotnet.exe'
if (!(Test-Path -LiteralPath $compiler) -or !(Test-Path -LiteralPath $runtime)) {
    throw 'Pass the Data folder of an installed Unity 6 Editor.'
}
$refs = @('/r:' + (Join-Path $EditorData 'NetStandard/ref/2.1.0/netstandard.dll'))
$refs += Get-ChildItem (Join-Path $EditorData 'Managed/UnityEngine') -Filter *.dll | ForEach-Object { '/r:' + $_.FullName }
$sources = Get-ChildItem 'unity/com.ludiars.tela/Editor' -Filter *.cs | ForEach-Object FullName
New-Item -ItemType Directory -Force 'build-unity' | Out-Null
& $runtime $compiler /nologo /target:library /nostdlib+ /out:build-unity/Tela.Editor.dll $refs $sources
exit $LASTEXITCODE
