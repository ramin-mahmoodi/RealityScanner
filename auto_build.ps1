$ErrorActionPreference = "Stop"

$cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v3.29.2/cmake-3.29.2-windows-x86_64.zip"
$cmakeZip = "cmake.zip"
$cmakeDir = "cmake_bin"

if (-not (Test-Path "$cmakeDir\bin\cmake.exe")) {
    Write-Host "Downloading CMake portable..."
    Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeZip
    Write-Host "Extracting CMake..."
    Expand-Archive -Path $cmakeZip -DestinationPath . -Force
    Rename-Item -Path "cmake-3.29.2-windows-x86_64" -NewName $cmakeDir
    Remove-Item $cmakeZip
}

$cmakeExe = (Resolve-Path "$cmakeDir\bin\cmake.exe").Path

Write-Host "Configuring project with CMake and MinGW..."
if (-not (Test-Path build)) {
    New-Item -ItemType Directory -Path build | Out-Null
}
Set-Location build
& $cmakeExe -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed."
    exit 1
}

Write-Host "Building project..."
& $cmakeExe --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed."
    exit 1
}

Write-Host "`n=============================================="
Write-Host "Build succeeded!"
Write-Host "Executable: build\RealityScanner.exe"
Write-Host "=============================================="
