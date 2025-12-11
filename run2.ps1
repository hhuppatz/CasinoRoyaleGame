# Run two instances of CasinoRoyale for multiplayer testing
# Usage: .\run2.ps1

Write-Host "Launching 2 instances of CasinoRoyale..." -ForegroundColor Cyan

# Check if executable exists
if (-not (Test-Path "build\bin\CasinoRoyale.exe")) {
    Write-Host "[ERROR] CasinoRoyale.exe not found! Run .\build.ps1 first." -ForegroundColor Red
    exit 1
}

# Launch first instance (Host)
Write-Host "  [1] Launching instance 1 (Host)..." -ForegroundColor Yellow
Start-Process -FilePath "build\bin\CasinoRoyale.exe" -WorkingDirectory $PWD
Start-Sleep -Milliseconds 500

# Launch second instance (Client)
Write-Host "  [2] Launching instance 2 (Client)..." -ForegroundColor Yellow
Start-Process -FilePath "build\bin\CasinoRoyale.exe" -WorkingDirectory $PWD

Write-Host "`n[SUCCESS] Both instances launched!" -ForegroundColor Green
Write-Host "  Tip: Use one as the host (start server) and one as the client (connect)" -ForegroundColor Gray

