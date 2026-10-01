param(
    [string]$Data = "runtime/inventory.csv",
    [string]$Seed = "cpp/product_inventory_10 000.csv",
    [int]$Port = 8081
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$server = Join-Path $projectRoot "build\bin\warehouse_server.exe"
if (-not (Test-Path $server)) {
    & (Join-Path $PSScriptRoot "build.ps1") -Configuration Debug
}
Set-Location $projectRoot
& $server --data $Data --seed $Seed --port $Port
exit $LASTEXITCODE
