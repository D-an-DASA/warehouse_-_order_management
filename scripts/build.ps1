param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$binDirectory = Join-Path $projectRoot "build\bin"
New-Item -ItemType Directory -Force -Path $binDirectory | Out-Null
Set-Location $projectRoot

$commonFlags = @("-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror=return-type")
$configFlags = if ($Configuration -eq "Release") {
    @("-O2")
} else {
    @("-O0", "-g")
}

function Build-Target {
    param(
        [string]$Name,
        [string[]]$Sources,
        [string[]]$Libraries = @()
    )
    Write-Host "[BUILD] $Name ($Configuration)"
    $output = Join-Path $binDirectory "$Name.exe"
    $compilerArguments = @()
    $compilerArguments += $commonFlags
    $compilerArguments += $configFlags
    $compilerArguments += $Sources
    $compilerArguments += @("-o", $output)
    $compilerArguments += $Libraries
    & g++ $compilerArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Bien dich $Name that bai."
    }
}

Build-Target "HashTableTest" @(
    "cpp/DSAcore/hashtable/HashTableTest.cpp",
    "cpp/DSAcore/hashtable/hashtable.cpp"
)
Build-Target "TrieTest" @(
    "cpp/DSAcore/trie/TrieTest.cpp",
    "cpp/DSAcore/trie/trie.cpp"
)
Build-Target "LRU_CacheTest" @("cpp/DSAcore/LRU_Cache/LRU_CacheTest.cpp")
Build-Target "Min_heaptest" @(
    "cpp/DSAcore/Min_Heap/Min_heaptest.cpp",
    "cpp/DSAcore/Min_Heap/Min_heap.cpp"
)
Build-Target "SearchCoreTest" @(
    "cpp/DSAcore/SearchCoreTest.cpp",
    "cpp/DSAcore/SearchCore.cpp",
    "cpp/DSAcore/hashtable/hashtable.cpp",
    "cpp/DSAcore/trie/trie.cpp",
    "cpp/DSAcore/Min_Heap/Min_heap.cpp"
)
Build-Target "CsvProductRepositoryTest" @(
    "cpp/Persistence/CsvProductRepositoryTest.cpp",
    "cpp/Persistence/CsvProductRepository.cpp"
)
Build-Target "warehouse_server" @(
    "cpp/server.cpp",
    "cpp/DSAcore/SearchCore.cpp",
    "cpp/DSAcore/hashtable/hashtable.cpp",
    "cpp/DSAcore/trie/trie.cpp",
    "cpp/DSAcore/Min_Heap/Min_heap.cpp",
    "cpp/Persistence/CsvProductRepository.cpp"
) @("-lws2_32")
Build-Target "Benchmark" @(
    "cpp/benchmark/Benchmark.cpp",
    "cpp/DSAcore/SearchCore.cpp",
    "cpp/DSAcore/hashtable/hashtable.cpp",
    "cpp/DSAcore/trie/trie.cpp",
    "cpp/DSAcore/Min_Heap/Min_heap.cpp",
    "cpp/Persistence/CsvProductRepository.cpp"
)

Write-Host "Build hoan tat: $binDirectory"
