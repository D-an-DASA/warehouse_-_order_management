$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot

& (Join-Path $PSScriptRoot "build.ps1") -Configuration Debug

$unitTests = @(
    "HashTableTest",
    "TrieTest",
    "LRU_CacheTest",
    "Min_heaptest",
    "SearchCoreTest",
    "CsvProductRepositoryTest"
)
foreach ($test in $unitTests) {
    Write-Host "[TEST] $test"
    & (Join-Path $projectRoot "build\bin\$test.exe")
    if ($LASTEXITCODE -ne 0) { throw "$test that bai." }
}

Write-Host "[TEST] API integration"
& (Join-Path $projectRoot "tests\api_integration.ps1")

$localPlaywright = Join-Path $projectRoot "node_modules\playwright"
$bundledNodeModules = Join-Path $env:USERPROFILE `
    ".cache\codex-runtimes\codex-primary-runtime\dependencies\node\node_modules"
if (-not (Test-Path $localPlaywright)) {
    if (Test-Path (Join-Path $bundledNodeModules "playwright")) {
        $env:NODE_PATH = $bundledNodeModules
    } else {
        throw "Chua co Playwright. Hay chay npm install va npx playwright install chromium."
    }
}

Write-Host "[TEST] Web E2E"
& node (Join-Path $projectRoot "tests\web_e2e.js")
if ($LASTEXITCODE -ne 0) { throw "Web E2E that bai." }

Write-Host "Tat ca kiem thu: PASS"
