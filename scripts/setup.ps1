# HorrorGengine セットアップ（コマンドライン一括実行）
# 要: Visual Studio 2022 (C++), CMake（ReactPhysics3D 用）, PowerShell 5.1+
# 推奨: "Developer PowerShell for VS 2022" で実行（msbuild が PATH に入っている）

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root

# MSBuild を取得（vswhere 使用）
$vs = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vs)) { throw "Visual Studio が見つかりません。vswhere のパスを確認してください。" }
$msbuild = & $vs -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild が見つかりません。'Developer PowerShell for VS 2022' で実行するか、VS の C++ ワークロードを入れてください。" }

Write-Host "=== 1. NuGet パッケージの復元 ===" -ForegroundColor Cyan
& $msbuild HorrorEngine.sln -t:restore -p:RestorePackagesConfig=true -v:m
if (-not (Test-Path "packages\AssimpCpp.5.0.1.6") -or -not (Test-Path "packages\assimp_native.redist.4.0.1")) {
    # packages.config の Assimp 依存は msbuild -t:restore で入らないことがある
    $nuget = Get-Command nuget -ErrorAction SilentlyContinue
    if (-not $nuget) {
        $np = Join-Path $root "nuget.exe"
        if (-not (Test-Path $np)) {
            Write-Host "nuget.exe を取得しています..."
            Invoke-WebRequest "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" -OutFile $np -UseBasicParsing
        }
        & $np restore HorrorEngine.sln
    } else {
        & nuget restore HorrorEngine.sln
    }
}

Write-Host "`n=== 2. ReactPhysics3D（include + lib） ===" -ForegroundColor Cyan
& (Join-Path $PSScriptRoot "fetch_reactphysics3d.ps1")
& (Join-Path $PSScriptRoot "build_reactphysics3d.ps1")

Write-Host "`n=== 3. DirectXTex（lib） ===" -ForegroundColor Cyan
$dxProj = "external\DirectXTex-main\DirectXTex\DirectXTex_Desktop_2022_Win10.vcxproj"
& $msbuild $dxProj -p:Configuration=Debug -p:Platform=x64 -v:m
& $msbuild $dxProj -p:Configuration=Release -p:Platform=x64 -v:m
$dxOut = "external\DirectXTex-main\DirectXTex\Bin\Desktop_2022_Win10\x64"
New-Item -ItemType Directory -Force -Path "external\directxtex\lib\x64\Debug" | Out-Null
New-Item -ItemType Directory -Force -Path "external\directxtex\lib\x64\Release" | Out-Null
Copy-Item "$dxOut\Debug\DirectXTex.lib" "external\directxtex\lib\x64\Debug\" -Force
Copy-Item "$dxOut\Release\DirectXTex.lib" "external\directxtex\lib\x64\Release\" -Force

Write-Host "`n=== 4. SoLoud（lib） ===" -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path "external\soloud\lib\x64\Debug", "external\soloud\lib\x64\Release" | Out-Null
$slProj = "soloud20200207\build\vs2022\SoloudStatic.vcxproj"
& $msbuild $slProj -p:Configuration=Debug -p:Platform=x64 -v:m
Copy-Item "soloud20200207\build\vs2022\x64\Debug\soloud_static.lib" "external\soloud\lib\x64\Debug\" -Force
& $msbuild $slProj -p:Configuration=Release -p:Platform=x64 -v:m
Copy-Item "soloud20200207\build\vs2022\x64\Release\soloud_static.lib" "external\soloud\lib\x64\Release\" -Force

Write-Host "`n=== 5. HorrorGengine のビルド ===" -ForegroundColor Cyan
& $msbuild HorrorEngine.sln -p:Configuration=Debug -p:Platform=x64 -v:m

Write-Host "`nセットアップ完了。Game\Game.exe を実行してください。" -ForegroundColor Green
