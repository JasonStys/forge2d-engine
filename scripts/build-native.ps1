# File: build-native.ps1
# Purpose: Build/package a static-runtime native Windows editor; never install or touch user profiles.
param([string]$Output = 'release/native-candidate')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Repo = Split-Path -Parent $PSScriptRoot
$Destination = [System.IO.Path]::GetFullPath((Join-Path $Repo $Output))
if (-not $Destination.StartsWith(($Repo + [System.IO.Path]::DirectorySeparatorChar), [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package output must remain inside this repository.'
}
if (Test-Path -LiteralPath $Destination) { throw 'Use a fresh package output; existing candidates are preserved.' }
cmake -S $Repo -B (Join-Path $Repo '.runtime/native-build') -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
if ($LASTEXITCODE -ne 0) { throw 'Native configuration failed.' }
cmake --build (Join-Path $Repo '.runtime/native-build') --parallel 2
if ($LASTEXITCODE -ne 0) { throw 'Native build failed.' }
ctest --test-dir (Join-Path $Repo '.runtime/native-build') --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Native/core tests failed.' }
$Package = Join-Path $Destination 'Forge2DEditor'
New-Item -ItemType Directory -Path (Join-Path $Package 'assets') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $Repo '.runtime/native-build/Forge2DEditor.exe') -Destination $Package
Copy-Item -LiteralPath (Join-Path $Repo 'LICENSE') -Destination $Package
foreach ($Name in @('arena.scene', 'collision-layers.scene')) {
    Copy-Item -LiteralPath (Join-Path $Repo "assets/$Name") -Destination (Join-Path $Package 'assets')
}
$Metadata = (Get-Item -LiteralPath (Join-Path $Package 'Forge2DEditor.exe')).VersionInfo
if ($Metadata.CompanyName -ne 'Jason Stys' -or $Metadata.ProductVersion -ne '1.1.0') { throw 'Native author/version mismatch.' }
$Source = (git -C $Repo -c "safe.directory=$Repo" rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $Source -notmatch '^[0-9a-f]{40}$') { throw 'Source identity unavailable.' }
$Dirty = @(git -C $Repo -c "safe.directory=$Repo" status --porcelain).Count -ne 0
@{schema=1; version='1.1.0'; source=$Source; dirty=$Dirty; format='Windows portable'; author='Jason Stys'} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $Package 'RELEASE.json') -Encoding utf8
@'
Forge2D Editor 1.1.0 - Jason Stys
Extract this entire folder, then launch Forge2DEditor.exe. No IDE, compiler, Python or Node required.
Help / About describes all controls and scene fields. Prepared data is inside this actual editor.
Your project is saved under your OS-local Forge2DEditor profile, outside this program folder.
No hardware, network, authentication or physical collision-response guarantee. Unsigned portable app.
Source and contact: https://github.com/JasonStys/forge2d-engine
'@ | Set-Content -LiteralPath (Join-Path $Package 'README.txt') -Encoding utf8
$Archive = Join-Path $Destination 'Forge2DEditor-1.1.0-windows-x64.zip'
Compress-Archive -Path (Join-Path $Package '*') -DestinationPath $Archive -CompressionLevel Optimal
$Stream = [System.IO.File]::OpenRead($Archive)
$Hasher = [System.Security.Cryptography.SHA256]::Create()
try { $Hash = [BitConverter]::ToString($Hasher.ComputeHash($Stream)).Replace('-', '').ToLowerInvariant() }
finally { $Stream.Dispose(); $Hasher.Dispose() }
"$Hash  $([System.IO.Path]::GetFileName($Archive))" | Set-Content -LiteralPath ($Archive + '.sha256') -Encoding ascii
Write-Output $Archive
