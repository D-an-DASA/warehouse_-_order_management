# DASA — Warehouse & Order Management

Đồ án minh họa cách phối hợp các cấu trúc dữ liệu tự cài đặt trong quy trình
tra cứu và chuẩn bị sản phẩm trong kho. Phạm vi hiện tại quản lý từng Product;
chưa xây dựng mô hình Order đầy đủ.

## Luồng chức năng và cấu trúc dữ liệu

| Thao tác trên Web | Cấu trúc chính | Hành vi |
|---|---|---|
| Tìm ID chính xác | HashTable | Trả sản phẩm ở mọi trạng thái và ghi READ vào LRU |
| Gợi ý theo tiền tố | Trie | Trả tối đa 20 tên duy nhất |
| Xem danh sách ưu tiên | Trie + Min Heap | Chỉ lấy AVAILABLE, xếp theo best_by_date, arrived_time, id |
| Chuẩn bị đơn hàng | HashTable | Đổi AVAILABLE thành RESERVED, lưu CSV và ghi RESERVE |
| Xem thao tác gần đây | LRU Cache | Tối đa 50 sản phẩm, mới nhất đứng trước |
| Xóa vĩnh viễn | HashTable + Trie | Xóa khỏi hai chỉ mục và CSV qua khung quản lý riêng |

previewPriority không làm thay đổi trạng thái. Min Heap được dựng tạm cho mỗi
lần xem danh sách nên con trỏ trong heap không tồn tại qua lần thay đổi HashTable.
Khi người dùng bấm **Chuẩn bị đơn hàng**, server cập nhật sản phẩm trong HashTable
thành RESERVED; lần tải lại tiếp theo lọc sản phẩm đó khỏi Min Heap.

## Kiến trúc ba lớp

    Web (HTML/CSS/JavaScript)
            | HTTP/JSON
            v
    server.cpp (API + khóa đồng bộ + persistence)
            |
            v
    SearchCore (HashTable + Trie + Min Heap + LRU Cache)
            |
            v
    CsvProductRepository (runtime/inventory.csv)

Hai file cpp/product_inventory_10 000.csv và
cpp/product_inventory_100 000.csv chỉ là seed/benchmark, không bị Web ghi đè.
Lần chạy đầu tạo bản làm việc riêng tại runtime/inventory.csv.

## Build và chạy trên Windows

Yêu cầu: PowerShell, g++ hỗ trợ C++17 và Python hoặc một static web server.

    .\scripts\build.ps1 -Configuration Debug
    .\scripts\run.ps1

Trong terminal khác:

    python -m http.server 5500 --directory web

Mở http://localhost:5500. Backend mặc định ở http://localhost:8081.
Có thể đổi backend cho Web bằng query string api=http://localhost:18081.

## Kiểm thử và benchmark

    .\scripts\test.ps1
    .\scripts\benchmark.ps1 -Repeats 5 -Seed 20261001

Web E2E cần Playwright và Chromium, Chrome hoặc Edge. Nếu máy chưa có:

    npm install
    npx playwright install chromium

Kết quả benchmark nằm tại
[benchmark/results/summary.md](benchmark/results/summary.md), dữ liệu từng lần
đo nằm trong [benchmark/results/raw.csv](benchmark/results/raw.csv).

## Tài liệu

- [Hợp đồng API](docs/API.md)
- [Cách kiểm thử và bằng chứng](docs/TESTING.md)
- [Kịch bản demo](docs/DEMO.md)
- [Nhật ký debug](docs/DEBUG_LOG.md)
- [Audit việc sử dụng AI](docs/AI_AUDIT.md)

## Cấu trúc chính

    cpp/
      DSAcore/             # HashTable, Trie, Min Heap, LRU, SearchCore
      Persistence/         # Đọc/ghi CSV
      benchmark/           # Chương trình benchmark tương đương
      server.cpp           # HTTP API
    web/                   # Giao diện
    scripts/               # Build, run, test, benchmark
    tests/                 # API integration, Web E2E và fixture
    benchmark/results/     # Số liệu thô, tổng hợp và môi trường

File build, executable, runtime, báo cáo Playwright và node_modules không được
đưa vào Git.
