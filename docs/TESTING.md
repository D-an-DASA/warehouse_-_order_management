# Kiểm thử và bằng chứng

## Chạy toàn bộ

    .\scripts\test.ps1

Script build Debug rồi chạy:

1. HashTableTest: insert, duplicate, resize, lookup, remove và snapshot.
2. TrieTest: prefix, nhiều ID, remove và not-found.
3. LRU_CacheTest: dung lượng 50, eviction, move-to-front và remove.
4. Min_heaptest: thứ tự best_by_date, arrived_time, id.
5. SearchCoreTest: MC1, MC2, reserve, validation, đồng bộ chỉ mục và LRU.
6. CsvProductRepositoryTest: round-trip, CSV lỗi và số dòng 10k/100k.
7. api_integration.ps1: API thật, CSV tạm và kiểm tra lại sau restart.
8. web_e2e.js: Chromium thật thao tác autocomplete, Min Heap, reserve,
   exact-ID, LRU và khung xóa riêng.

Các lần chạy riêng ngày 2026-10-01 đã đạt:

- 6/6 executable unit test.
- SearchCoreTest: 8/8 nhóm kiểm thử.
- API integration: PASS.
- Web E2E: PASS.

Lệnh tổng `.\\scripts\\test.ps1` sau cùng cũng kết thúc với
`Tat ca kiem thu: PASS`.

## Benchmark

    .\scripts\benchmark.ps1 -Repeats 5 -Seed 20261001

- Warm-up: 1 lượt không ghi số.
- Đo: 5 lượt.
- MC1 cùng exact-ID workload và checksum.
- MC2 cùng prefix, điều kiện AVAILABLE, comparator và limit 20.
- Kết quả raw: benchmark/results/raw.csv.
- Median: benchmark/results/summary.md.
- Môi trường: benchmark/results/environment.md.

Không dùng thời gian nạp CSV hoặc dựng chỉ mục để thay cho thời gian query; hai
pha setup được ghi thành dòng riêng.
