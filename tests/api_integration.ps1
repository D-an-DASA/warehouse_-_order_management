param([int]$Port = 18081)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$serverExe = Join-Path $projectRoot "build\bin\warehouse_server.exe"
$fixture = Join-Path $PSScriptRoot "fixtures\inventory.csv"
$tempRoot = [IO.Path]::GetTempPath()
$tempDirectory = Join-Path $tempRoot ("warehouse_api_test_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempDirectory | Out-Null
$dataFile = Join-Path $tempDirectory "inventory.csv"
Copy-Item -LiteralPath $fixture -Destination $dataFile
$baseUrl = "http://localhost:$Port"
$serverProcess = $null

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

function Start-TestServer {
    $arguments = @("--data", $dataFile, "--seed", $fixture, "--port", "$Port")
    $script:serverProcess = Start-Process -FilePath $serverExe -ArgumentList $arguments `
        -PassThru -WindowStyle Hidden
    for ($attempt = 0; $attempt -lt 50; $attempt++) {
        try {
            return Invoke-RestMethod "$baseUrl/health"
        } catch {
            Start-Sleep -Milliseconds 200
        }
    }
    throw "Server khong san sang tai $baseUrl."
}

function Stop-TestServer {
    if ($null -ne $script:serverProcess -and -not $script:serverProcess.HasExited) {
        Stop-Process -Id $script:serverProcess.Id
        Wait-Process -Id $script:serverProcess.Id -ErrorAction SilentlyContinue
    }
    $script:serverProcess = $null
}

try {
    $health = Start-TestServer
    Assert-True ($health.products -eq 4) "Health phai bao 4 san pham."

    $suggestions = Invoke-RestMethod "$baseUrl/search/autocomplete?prefix=Pow"
    Assert-True ($suggestions.Count -eq 1 -and $suggestions[0] -eq "Power Bank") `
        "Autocomplete khong dung."

    $priority = Invoke-RestMethod "$baseUrl/search/result?query=Power"
    Assert-True ($priority.mode -eq "priority") "Sai che do tim tien to."
    Assert-True ($priority.results.Count -eq 2) "MC2 phai loc chi AVAILABLE."
    Assert-True ($priority.results[0].id -eq "P00002") "Min Heap sai thu tu uu tien."

    $reserveBody = @{ id = "P00002" } | ConvertTo-Json
    $reserved = Invoke-RestMethod "$baseUrl/product/reserve" -Method Post `
        -ContentType "application/json" -Body $reserveBody
    Assert-True ($reserved.product.status -eq "RESERVED") "Reserve khong doi trang thai."

    $priorityAfterReserve = Invoke-RestMethod "$baseUrl/search/result?query=Power"
    Assert-True ($priorityAfterReserve.results.Count -eq 1) `
        "San pham RESERVED chua roi khoi danh sach Min Heap."
    Assert-True ($priorityAfterReserve.results[0].id -eq "P00001") `
        "Danh sach sau reserve khong dung."

    $exact = Invoke-RestMethod "$baseUrl/search/result?query=P00002"
    Assert-True ($exact.results[0].status -eq "RESERVED") `
        "HashTable chua cap nhat RESERVED."

    $deleteBody = @{ id = "P00001" } | ConvertTo-Json
    $deleted = Invoke-RestMethod "$baseUrl/product/delete" -Method Delete `
        -ContentType "application/json" -Body $deleteBody
    Assert-True ($deleted.deleted_id -eq "P00001") "API xoa tra ket qua sai."

    Stop-TestServer
    $healthAfterRestart = Start-TestServer
    Assert-True ($healthAfterRestart.products -eq 3) "Xoa chua duoc luu sau restart."
    $exactAfterRestart = Invoke-RestMethod "$baseUrl/search/result?query=P00002"
    Assert-True ($exactAfterRestart.results[0].status -eq "RESERVED") `
        "RESERVED chua duoc luu sau restart."

    Write-Host "api_integration: PASS"
} finally {
    Stop-TestServer
    $resolvedTemp = [IO.Path]::GetFullPath($tempDirectory)
    $resolvedRoot = [IO.Path]::GetFullPath($tempRoot)
    if ($resolvedTemp.StartsWith($resolvedRoot) -and
        [IO.Path]::GetFileName($resolvedTemp).StartsWith("warehouse_api_test_")) {
        Remove-Item -LiteralPath $resolvedTemp -Recurse -Force -ErrorAction SilentlyContinue
    }
}
