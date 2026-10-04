# Kết quả benchmark MC1 và MC2

## Môi trường thực hiện

- Bộ xử lý: AMD Ryzen 7 5800H with Radeon Graphics
- Hệ điều hành: Windows
- Trình biên dịch: g++ 15.2.0
- Các cờ biên dịch: `-std=c++17 -O2 -Wall -Wextra -pedantic`
- Số lần chạy khởi động (`warm-up`): 3
- Số lần chạy được đo: 15
- Số truy vấn MC1 trong mỗi lần chạy: 200
- Số kết quả MC2: 100 sản phẩm có trạng thái `AVAILABLE`

Các giá trị thời gian bên dưới sử dụng đơn vị micro giây (`µs`) và giữ nguyên dấu chấm thập phân theo kết quả do chương trình benchmark xuất ra.

## MC1 – Tìm kiếm chính xác theo mã sản phẩm

MC1 so sánh việc tìm kiếm chính xác sản phẩm theo mã định danh (`ID`) bằng hai phương pháp:

- Duyệt tuần tự toàn bộ danh sách (`Linear scan`).
- Sử dụng cấu trúc `HashTable` do nhóm tự xây dựng.

Hai phương pháp nhận cùng một danh sách truy vấn, bao gồm cả những mã sản phẩm tồn tại và không tồn tại trong dữ liệu.

| Số bản ghi | Phương pháp | Trung bình (µs/truy vấn) | Trung vị (µs/truy vấn) | Hệ số tăng tốc theo trung vị |
|---:|---|---:|---:|---:|
| 10.000 | Linear scan | 31.090 | 33.802 | 1.00x |
| 10.000 | HashTable | 0.036 | 0.036 | 938.931x |
| 100.000 | Linear scan | 301.919 | 293.329 | 1.00x |
| 100.000 | HashTable | 0.038 | 0.038 | 7719.184x |

### Nhận xét về MC1

- `Linear scan` phải kiểm tra lần lượt các sản phẩm nên có độ phức tạp kỳ vọng là `O(n)`.
- Tìm kiếm bằng `HashTable` có độ phức tạp trung bình kỳ vọng là `O(1)`.
- Khi số bản ghi tăng từ 10.000 lên 100.000, thời gian tìm kiếm bằng `Linear scan` tăng đáng kể, trong khi thời gian tìm kiếm bằng `HashTable` gần như ổn định.
- Trước khi đo thời gian, chương trình kiểm tra để bảo đảm hai phương pháp trả về cùng một sản phẩm cho từng truy vấn.

## MC2 – Lấy sản phẩm theo thứ tự ưu tiên

MC2 so sánh việc lấy 100 sản phẩm đầu tiên có trạng thái `AVAILABLE` bằng hai phương pháp:

- Sử dụng cấu trúc `MinHeap` do nhóm tự xây dựng.
- Sử dụng phương pháp đơn giản (`Naive`), liên tục duyệt các sản phẩm chưa được chọn để tìm sản phẩm có độ ưu tiên cao nhất tiếp theo.

Thứ tự ưu tiên được xác định như sau:

1. `best_by_date` sớm hơn.
2. Nếu `best_by_date` bằng nhau, ưu tiên `arrived_time` sớm hơn.
3. Nếu hai giá trị trên bằng nhau, ưu tiên mã sản phẩm (`ID`) nhỏ hơn.

| Số bản ghi | Số sản phẩm AVAILABLE | Phương pháp | Trung bình (µs/lần chạy) | Trung vị (µs/lần chạy) | Hệ số tăng tốc theo trung vị |
|---:|---:|---|---:|---:|---:|
| 10.000 | 3.508 | Naive | 4766.940 | 4453.500 | 1.00x |
| 10.000 | 3.508 | MinHeap | 191.747 | 186.600 | 23.867x |
| 100.000 | 34.539 | Naive | 56534.633 | 55295.600 | 1.00x |
| 100.000 | 34.539 | MinHeap | 2049.360 | 2026.500 | 27.286x |

### Nhận xét về MC2

- Phương pháp `Naive` thực hiện tối đa `K` lần duyệt toàn bộ các sản phẩm ứng viên nên có độ phức tạp xấp xỉ `O(Kn)`.
- `MinHeap` hiện được xây dựng bằng cách thêm lần lượt từng sản phẩm, vì vậy quá trình tạo heap có độ phức tạp `O(n log n)`.
- Quá trình lấy `K` sản phẩm từ `MinHeap` có độ phức tạp `O(K log n)`.
- Hai phương pháp sử dụng cùng tập sản phẩm có trạng thái `AVAILABLE` và cùng quy tắc so sánh độ ưu tiên.
- Trước khi đo hiệu năng, chương trình kiểm tra để bảo đảm hai phương pháp trả về cùng danh sách mã sản phẩm và đúng thứ tự.

## Kết luận

Kết quả cho thấy `HashTable` tìm kiếm chính xác theo mã sản phẩm nhanh hơn đáng kể so với `Linear scan`. Khi dữ liệu tăng từ 10.000 lên 100.000 bản ghi, thời gian trung vị của `Linear scan` tăng rõ rệt, trong khi thời gian của `HashTable` gần như không thay đổi. Đối với MC2, `MinHeap` nhanh hơn phương pháp `Naive` khoảng 24 đến 27 lần khi lấy 100 sản phẩm có trạng thái `AVAILABLE`. Trước khi đo thời gian, chương trình đã xác minh rằng các phương pháp được so sánh trả về kết quả giống nhau.

## Cách biên dịch và chạy lại benchmark

Mở PowerShell tại thư mục gốc của dự án và chạy:

```powershell
New-Item -ItemType Directory -Force build | Out-Null

g++ -std=c++17 -O2 -Wall -Wextra -pedantic `
  cpp/benchmarks/MC1_MC2_Benchmark.cpp `
  cpp/DSAcore/hashtable/hashtable.cpp `
  cpp/DSAcore/Min_heap/Min_heap.cpp `
  -o build/mc1_mc2_benchmark.exe

.\build\mc1_mc2_benchmark.exe `
  'cpp/product_inventory_10 000.csv' `
  'cpp/product_inventory_100 000.csv'
```

Benchmark chỉ được xem là hoàn thành khi chương trình đọc đúng 10.000 và 100.000 bản ghi, kiểm tra thành công tính đúng đắn của MC1 và MC2, đồng thời in ra thông báo:

```text
BENCHMARK COMPLETED SUCCESSFULLY
```