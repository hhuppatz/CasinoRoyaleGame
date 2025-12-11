# Build script for CasinoRoyale
# Usage: .\build.ps1

Write-Host "Building CasinoRoyale..." -ForegroundColor Cyan

# Navigate to build directory
Set-Location build

# Build the project
cmake --build .

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] Build successful!" -ForegroundColor Green
    Write-Host "Executable location: build\bin\CasinoRoyale.exe" -ForegroundColor Gray
} else {
    Write-Host "`n[FAILED] Build failed!" -ForegroundColor Red
    exit 1
}

# Return to project root
Set-Location ..

