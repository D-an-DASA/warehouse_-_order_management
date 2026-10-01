param(
    [int]$Repeats = 5,
    [int]$Seed = 20261001
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $projectRoot
& (Join-Path $PSScriptRoot "build.ps1") -Configuration Release

$benchmarkExe = Join-Path $projectRoot "build\bin\Benchmark.exe"
$resultDirectory = Join-Path $projectRoot "benchmark\results"
New-Item -ItemType Directory -Force -Path $resultDirectory | Out-Null

$datasets = @(
    @{ Path = "cpp/product_inventory_10 000.csv"; Label = "10k"; Records = 10000 },
    @{ Path = "cpp/product_inventory_100 000.csv"; Label = "100k"; Records = 100000 }
)
$allLines = [System.Collections.Generic.List[string]]::new()
foreach ($dataset in $datasets) {
    Write-Host "[BENCHMARK] $($dataset.Label)"
    $lines = & $benchmarkExe $dataset.Path $dataset.Label $dataset.Records $Repeats $Seed
    if ($LASTEXITCODE -ne 0) { throw "Benchmark $($dataset.Label) that bai." }
    if ($allLines.Count -eq 0) {
        $allLines.AddRange([string[]]$lines)
    } else {
        $allLines.AddRange([string[]]($lines | Select-Object -Skip 1))
    }
}

$rawPath = Join-Path $resultDirectory "raw.csv"
[IO.File]::WriteAllLines($rawPath, $allLines)
$rows = Import-Csv $rawPath
$summary = [System.Collections.Generic.List[string]]::new()
$summary.Add("# Kết quả benchmark")
$summary.Add("")
$summary.Add("Seed: ``$Seed``; warm-up: ``1``; số lần đo: ``$Repeats``.")
$summary.Add("")
$summary.Add("| Dataset | Tác vụ | Tối ưu (median ms) | Baseline (median ms) | Tăng tốc |")
$summary.Add("|---|---|---:|---:|---:|")

function Get-Median([object[]]$Values) {
    $sorted = @($Values | ForEach-Object { [double]$_ } | Sort-Object)
    $middle = [int][math]::Floor($sorted.Count / 2)
    if ($sorted.Count % 2 -eq 1) { return $sorted[$middle] }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2
}

foreach ($records in @(10000, 100000)) {
    foreach ($operation in @("mc1_exact_id", "mc2_priority")) {
        $group = @($rows | Where-Object {
            $_.phase -eq "query" -and [int]$_.records -eq $records -and
            $_.operation -eq $operation
        })
        $optimizedName = if ($operation -eq "mc1_exact_id") { "hash_table" } else { "trie_min_heap" }
        $baselineName = if ($operation -eq "mc1_exact_id") { "linear_scan" } else { "filter_sort" }
        $optimized = Get-Median @($group | Where-Object implementation -eq $optimizedName |
            Select-Object -ExpandProperty elapsed_ns)
        $baseline = Get-Median @($group | Where-Object implementation -eq $baselineName |
            Select-Object -ExpandProperty elapsed_ns)
        $optimizedMs = $optimized / 1000000
        $baselineMs = $baseline / 1000000
        $speedup = if ($optimized -gt 0) { $baseline / $optimized } else { 0 }
        $summary.Add(("| {0:N0} | {1} | {2:N3} | {3:N3} | {4:N2}x |" -f `
            $records, $operation, $optimizedMs, $baselineMs, $speedup))
    }
}
$summary.Add("")
$summary.Add("Dữ liệu thô: [raw.csv](raw.csv). Thời gian nạp CSV và dựng chỉ mục được ghi riêng với ``phase=setup``.")
[IO.File]::WriteAllLines((Join-Path $resultDirectory "summary.md"), $summary)

$compiler = (& g++ --version | Select-Object -First 1)
$environment = @(
    "# Môi trường benchmark",
    "",
    "- Ngày chạy: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')",
    "- Hệ điều hành: $([Environment]::OSVersion.VersionString)",
    "- CPU: $env:PROCESSOR_IDENTIFIER",
    "- Compiler: $compiler",
    "- Cấu hình: C++17, ``-O2``",
    "- Seed: $Seed",
    "- Warm-up: 1",
    "- Repeats: $Repeats"
)
[IO.File]::WriteAllLines((Join-Path $resultDirectory "environment.md"), $environment)
Write-Host "Benchmark hoan tat: $resultDirectory"
