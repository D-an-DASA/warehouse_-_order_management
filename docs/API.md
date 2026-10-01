# Hợp đồng HTTP API

Backend mặc định: http://localhost:8081. Mọi phản hồi lỗi có dạng:

    {"error":{"code":"ERROR_CODE","message":"Mô tả"}}

## Tra cứu

### GET /search/autocomplete?prefix=Pow&limit=20

Trả mảng tối đa 20 tên sản phẩm duy nhất từ Trie.

### GET /search/result?query=P00001&limit=50

- Query giống ID: HashTable exact lookup, mode exact; có thể trả AVAILABLE,
  RESERVED hoặc EXPIRED.
- Query tên/tiền tố: Trie lấy ứng viên, chỉ AVAILABLE được đưa vào Min Heap,
  mode priority.

    {
      "query": "Power",
      "mode": "priority",
      "results": [{
        "id": "P00002",
        "product_name": "Power Bank",
        "made_date": "2026-01-01",
        "arrived_time": "2026-01-02 08:00:00",
        "best_by_date": "2026-11-15",
        "status": "AVAILABLE"
      }]
    }

## Thao tác sản phẩm

### POST /product/reserve

Body: {"id":"P00002"}. Chỉ chấp nhận sản phẩm AVAILABLE. Thành công đổi trạng
thái trong HashTable thành RESERVED, ghi persistence và LRU. Gọi lại với cùng ID
trả 409 RESERVE_REJECTED.

### DELETE /product/delete

Body: {"id":"P00001"}. Xóa vĩnh viễn khỏi HashTable, Trie và CSV. Chức năng này
thuộc khung quản lý riêng, không phải thao tác của Min Heap.

### POST /product/add

    {
      "product_name": "Power Adapter",
      "made_date": "2026-01-01",
      "arrived_time": "2026-01-02 08:00:00",
      "best_by_date": "2028-01-01",
      "quantity": 2
    }

quantity từ 1 đến 100. Mỗi đơn vị nhận một ID riêng và trạng thái AVAILABLE.

### GET /product/recent

Trả tối đa 50 mục LRU theo thứ tự mới nhất trước:

    [{"product":{"id":"P00002"},"operation":"RESERVE","time":"15:30:00"}]

### GET /health

Trả {"status":"ok","products":10000} để kiểm tra server và số sản phẩm.

## Mã trạng thái chính

| HTTP | Ý nghĩa |
|---:|---|
| 200/201 | Thành công |
| 400 | JSON, query hoặc dữ liệu đầu vào không hợp lệ |
| 404 | Không tìm thấy ID |
| 409 | Sản phẩm không ở trạng thái cho phép reserve |
| 500 | Không ghi được persistence |
