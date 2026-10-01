# Kịch bản demo

1. Chạy backend bằng .\scripts\run.ps1 và phục vụ thư mục web.
2. Nhập Pow; chỉ ra gợi ý Power Bank đến từ Trie.
3. Tìm Power; giải thích danh sách chỉ có AVAILABLE và được Min Heap xếp theo
   hạn dùng, thời gian nhập, ID.
4. Bấm **Chuẩn bị đơn hàng** ở dòng đầu. Dòng biến mất khỏi danh sách ưu tiên.
5. Tìm chính xác ID vừa chọn; HashTable vẫn trả sản phẩm với RESERVED.
6. Quan sát LRU có thao tác RESERVE ở đầu.
7. Dùng khung **Quản lý sản phẩm** để thêm một sản phẩm.
8. Dùng ô **Xóa sản phẩm khỏi HashTable** để xóa ID thử nghiệm; nhấn mạnh đây
   là chức năng riêng, không phải pop khỏi Min Heap.
9. Khởi động lại server và tìm ID đã reserve để chứng minh persistence.
10. Mở benchmark/results/summary.md và raw.csv để trình bày số liệu 10k/100k.

Không thao tác trực tiếp trên hai dataset benchmark trong lúc demo; server dùng
runtime/inventory.csv.
