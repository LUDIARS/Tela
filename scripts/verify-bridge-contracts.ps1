param([Parameter(Mandatory=$true)][string]$EditorData)
$ErrorActionPreference = 'Stop'
$u = [Text.UTF8Encoding]::new($false)
[Console]::OutputEncoding = $u
$OutputEncoding = $u
$runtime = Join-Path $EditorData 'NetCoreRuntime/dotnet.exe'
$compiler = Join-Path $EditorData 'DotNetSdkRoslyn/csc.dll'
$frameworkRoot = Join-Path $EditorData 'NetCoreRuntime/shared/Microsoft.NETCore.App'
$framework = Get-ChildItem -LiteralPath $frameworkRoot -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
if (!$framework -or !(Test-Path -LiteralPath $runtime) -or !(Test-Path -LiteralPath $compiler)) { throw 'Installed Unity 6 compiler/runtime required.' }
New-Item -ItemType Directory -Force 'build-unity' | Out-Null
$assemblies = @('System.Private.CoreLib', 'System.Runtime', 'System.Console', 'System.Collections',
    'System.Threading', 'System.Threading.Thread', 'System.IO.Pipes', 'System.Security.Principal',
    'System.Runtime.InteropServices', 'System.ComponentModel.Primitives')
$refs = $assemblies | ForEach-Object { '/r:' + (Join-Path $framework.FullName ($_.ToString() + '.dll')) }
$sources = @('tests/unity/BridgeContracts.cs', 'unity/com.ludiars.tela/Editor/SceneHostBinding.cs',
    'unity/com.ludiars.tela/Editor/SceneActivity.cs', 'unity/com.ludiars.tela/Editor/PipeConnection.cs')
& $runtime $compiler /nologo /target:exe /nostdlib+ /out:build-unity/Tela.BridgeContracts.dll $refs $sources
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$config = @{ runtimeOptions = @{ tfm = 'net6.0'; framework = @{ name = 'Microsoft.NETCore.App'; version = $framework.Name } } } | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText((Join-Path $PWD 'build-unity/Tela.BridgeContracts.runtimeconfig.json'), $config, $u)
# Pure contracts only: no Unity Editor, native overlay, or listening server is launched.
& $runtime 'build-unity/Tela.BridgeContracts.dll'
exit $LASTEXITCODE
