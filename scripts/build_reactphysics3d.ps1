# ReactPhysics3D をビルドし、external/reactphysics3d/lib に lib をコピーする
# 要: CMake, Visual Studio（C++）
# fetch_reactphysics3d.ps1 で include が未取得の場合は先に実行すること

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$url = "https://github.com/DanielChappuis/reactphysics3d/archive/refs/tags/v0.9.0.zip"
$zip = "$env:TEMP\rp3d_build.zip"
$ext = "$env:TEMP\rp3d_src"
$dst = Join-Path $root "external\reactphysics3d"

if (Test-Path $ext) { Remove-Item $ext -Recurse -Force }
New-Item -ItemType Directory -Force -Path $ext | Out-Null

Write-Host "Downloading ReactPhysics3D v0.9.0 (source)..."
Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
Expand-Archive -Path $zip -DestinationPath $ext -Force
$src = Get-ChildItem $ext -Directory | Select-Object -First 1 | ForEach-Object { $_.FullName }

$bld = Join-Path $src "build"
Write-Host "CMake configure..."
cmake -B $bld -G "Visual Studio 17 2022" -A x64 $src
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

Write-Host "CMake build (Debug)..."
cmake --build $bld --config Debug
Write-Host "CMake build (Release)..."
cmake --build $bld --config Release

$libD = Get-ChildItem -Path $bld -Recurse -Filter "*.lib" -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match "\\Debug\\" } | Select-Object -First 1
$libR = Get-ChildItem -Path $bld -Recurse -Filter "*.lib" -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match "\\Release\\" } | Select-Object -First 1

New-Item -ItemType Directory -Force -Path (Join-Path $dst "lib\x64\Debug") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $dst "lib\x64\Release") | Out-Null

if ($libD) { Copy-Item $libD.FullName (Join-Path $dst "lib\x64\Debug\reactphysics3d.lib") -Force; Write-Host "Copied Debug lib." }
if ($libR) { Copy-Item $libR.FullName (Join-Path $dst "lib\x64\Release\reactphysics3d.lib") -Force; Write-Host "Copied Release lib." }
if (-not $libD -and -not $libR) { Write-Warning "ビルド出力の .lib が見つかりません。CMake/Visual Studio のバージョンを確認してください。" }

# include もコピー（fetch 未実行や include 欠損時に備える）
$incSrc = Join-Path $src "include"
if (Test-Path $incSrc) {
    $incDst = Join-Path $dst "include"
    New-Item -ItemType Directory -Force -Path $incDst | Out-Null
    Copy-Item -Path "$incSrc\*" -Destination $incDst -Recurse -Force
    Write-Host "Copied include/."
}

Remove-Item $zip -Force -ErrorAction SilentlyContinue
Remove-Item $ext -Recurse -Force -ErrorAction SilentlyContinue
