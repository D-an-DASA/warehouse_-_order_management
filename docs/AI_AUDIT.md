# Audit việc sử dụng AI

AI được dùng để hỗ trợ rà yêu cầu, đề xuất thiết kế, viết test và phát hiện lỗi.
Nhóm vẫn phải đọc diff, chạy test và chịu trách nhiệm cho quyết định cuối cùng.

## Đề xuất đã chấp nhận

- Tách SearchCore, HTTP API và CsvProductRepository thành ba trách nhiệm.
- Làm API tìm kiếm stateless bằng query parameter thay cho biến tìm kiếm dùng
  chung giữa các trình duyệt.
- Dùng file runtime riêng và ghi qua file tạm trước khi thay file đích.
- Dùng DOM textContent cho dữ liệu API.
- So benchmark với baseline tương đương và checksum kết quả.

## Đề xuất đã điều chỉnh hoặc từ chối

- Đã từ chối giữ Min Heap lâu dài bằng Product* lấy từ HashTable: resize hoặc
  thay đổi bucket có thể làm con trỏ không còn an toàn. Thiết kế cuối dựng heap
  tạm trong một request có khóa, sao chép kết quả trước khi trả.
- Ban đầu previewPriority chỉ xem danh sách. Theo quyết định của người dùng,
  giao diện được bổ sung nút reserve trên từng dòng nhưng previewPriority vẫn
  là hàm chỉ đọc; reserve là API riêng.
- Không đồng nhất reserve với delete. Reserve giữ sản phẩm trong HashTable với
  trạng thái RESERVED; delete nằm ở khung riêng và xóa khỏi HashTable, Trie, CSV.
- Không tự sinh tài liệu cá nhân D1/D3/D6/D7 thay thành viên nhóm.

## Cách kiểm chứng

- Unit test kiểm tra comparator, resize, đồng bộ chỉ mục, LRU 50 và reserve.
- API integration kiểm tra persistence sau restart.
- Web E2E thao tác bằng Chromium thật.
- Benchmark lưu dữ liệu thô, môi trường, seed và checksum.

Các lỗi và cách sửa thực tế được ghi tại docs/DEBUG_LOG.md; không thêm lỗi giả
để làm dày bằng chứng.
