# ReactPhysics3D を取得して external/reactphysics3d に展開する
# ソース: https://github.com/DanielChappuis/reactphysics3d

$ErrorActionPreference = "Stop"
$repo = "https://github.com/DanielChappuis/reactphysics3d/archive/refs/tags/v0.9.0.zip"
$zip = "$env:TEMP\rp3d.zip"
$ext = "$env:TEMP\rp3d_ext"
$dst = Join-Path $PSScriptRoot "..\external\reactphysics3d"

if (-not (Test-Path $dst)) { New-Item -ItemType Directory -Force -Path $dst | Out-Null }

Write-Host "Downloading ReactPhysics3D v0.9.0..."
Invoke-WebRequest -Uri $repo -OutFile $zip -UseBasicParsing

Write-Host "Extracting..."
if (Test-Path $ext) { Remove-Item $ext -Recurse -Force }
Expand-Archive -Path $zip -DestinationPath $ext -Force

$fold = Get-ChildItem $ext -Directory | Select-Object -First 1
Copy-Item -Path "$($fold.FullName)\include\*" -Destination (Join-Path $dst "include") -Recurse -Force

# LICENSE
if (Test-Path "$($fold.FullName)\LICENSE") {
    Copy-Item "$($fold.FullName)\LICENSE" (Join-Path $dst "LICENSE") -Force
}

Remove-Item $zip -Force -ErrorAction SilentlyContinue
Remove-Item $ext -Recurse -Force -ErrorAction SilentlyContinue

Write-Host "Done. include/ is ready. Build the library and place reactphysics3d.lib into external/reactphysics3d/lib/"
