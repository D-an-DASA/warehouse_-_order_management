# Nhật ký debug

## Test reserve gọi sai helper

- Hiện tượng: SearchCoreTest không biên dịch vì thiếu tham số thời gian nhập.
- Nguyên nhân: test mới không dùng đúng chữ ký helper năm tham số.
- Sửa: truyền đầy đủ arrived_time và gọi loadProducts trực tiếp.
- Xác minh: SearchCoreTest đạt 8/8.

## std::byte xung đột Win32

- Hiện tượng: MSYS2 báo reference to byte is ambiguous khi nạp windows.h và
  Winsock.
- Nguyên nhân: một số header DSA cũ có using namespace std, trong khi Win32
  cũng định nghĩa byte.
- Sửa: dùng WIN32_LEAN_AND_MEAN, NOMINMAX và nạp httplib trước header DSA.
- Xác minh: repository test và server đều liên kết thành công bằng g++ 15.2.

## Build Release truyền sai đối số

- Hiện tượng: g++ yêu cầu -E hoặc -x vì hiểu đầu vào là stdin.
- Nguyên nhân: PowerShell splat nhiều nhóm tham số không ổn định khi nhóm cấu
  hình Release chỉ có một phần tử.
- Sửa: hợp nhất toàn bộ cờ, source, output và library vào một mảng đối số.
- Xác minh: Release build và benchmark 10k/100k hoàn tất.

## Playwright thiếu headless shell

- Hiện tượng: package Playwright có sẵn nhưng executable headless shell không có.
- Sửa: test tìm theo thứ tự biến môi trường, Chromium Playwright, Chrome và Edge
  hệ thống.
- Xác minh: web_e2e PASS bằng Chrome headless.
