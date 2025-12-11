# Run single instance of CasinoRoyale
# Usage: .\run.ps1

Write-Host "Launching CasinoRoyale..." -ForegroundColor Cyan

# Check if executable exists
if (-not (Test-Path "build\bin\CasinoRoyale.exe")) {
    Write-Host "[ERROR] CasinoRoyale.exe not found! Run .\build.ps1 first." -ForegroundColor Red
    exit 1
}

# Launch the game
Start-Process -FilePath "build\bin\CasinoRoyale.exe" -WorkingDirectory $PWD

Write-Host "[SUCCESS] Game launched!" -ForegroundColor Green

