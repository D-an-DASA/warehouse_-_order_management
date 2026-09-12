# DASA — Warehouse & Order Management

## 1. Nội dung dự án

> **Trạng thái: Chưa triển khai**

Dự án hướng tới việc xây dựng một hệ thống **Warehouse & Order Management**.

Phần nghiệp vụ cụ thể, logic quản lý kho, quản lý đơn hàng và cấu trúc dataset **chưa được xác định và triển khai ở giai đoạn hiện tại**.

Phần này sẽ được cập nhật khi dự án bắt đầu bước vào giai đoạn phát triển business logic.

---

## 2. Tổng quan dự án

Ở giai đoạn hiện tại, dự án tập trung xây dựng **nền tảng kỹ thuật** cho hệ thống.

Kiến trúc chính gồm:

* **C++ Backend** — xử lý dữ liệu phía server, validation và cập nhật dữ liệu.
* **HTML/CSS/JavaScript Frontend** — xây dựng giao diện dashboard và giao tiếp với backend thông qua HTTP.
* **JSON** — định dạng dữ liệu trao đổi giữa frontend và backend.
* **HTTP API** — cung cấp các endpoint để frontend giao tiếp với C++ backend.

### Kiến trúc hiện tại

```text
┌──────────────────────────────┐
│       HTML Dashboard         │
│      CSS + JavaScript        │
└──────────────┬───────────────┘
               │
               │ HTTP / JSON
               ▼
┌──────────────────────────────┐
│        C++ Backend           │
│                              │
│   cpp-httplib HTTP Server    │
│   JSON Parsing               │
│   Data Validation            │
│   Data Updating              │
└──────────────────────────────┘
```

### Luồng dữ liệu hiện tại

Đối với thao tác cập nhật dữ liệu:

```text
User Input
    ↓
DataInput.html
    ↓
dataInput.js
    ↓
api.js
    ↓
POST /data
    ↓
C++ Backend
    ↓
Parse JSON
    ↓
Validation
    ↓
Cập nhật dữ liệu phía server
    ↓
Trả về JSON Response
    ↓
JavaScript xử lý Response
```

Đối với việc lấy dữ liệu cho Dashboard:

```text
C++ Backend
    ↓
GET /data
    ↓
JSON
    ↓
JavaScript fetch()
    ↓
Dashboard Chart
```

---

## 3. Cấu trúc thư mục hiện tại

```text
DASA/
│
├── index.html
├── DataInput.html
│
├── CSS/
│   └── style.css
│
├── JavaScript/
│   ├── app.js
│   ├── header.js
│   ├── navbar.js
│   ├── sidebar.js
│   ├── dashboardChart.js
│   ├── dataInput.js
│   └── api.js
│
└── cpp/
    ├── server.cpp
    ├── httplib.h
    └── json.hpp
```

---

# 4. Chức năng của từng file

## Root

### `index.html`

Trang Dashboard chính của hệ thống.

Chức năng:

* Xây dựng cấu trúc HTML của Dashboard.
* Chứa khu vực nội dung chính.
* Chứa các container dành cho chart và các thành phần hiển thị dữ liệu.
* Load các JavaScript module cần thiết.
* Khởi tạo các thành phần giao diện dùng chung.

---

### `DataInput.html`

Trang nhập và cập nhật dữ liệu.

Chức năng:

* Cung cấp giao diện nhập dữ liệu.
* Cho phép chọn `category`.
* Cho phép nhập `amount`.
* Cung cấp thao tác **Add** và **Remove**.
* Load các JavaScript cần thiết để giao tiếp với backend.

Luồng hiện tại:

```text
User
 ↓
Chọn Category
 ↓
Nhập Amount
 ↓
Add / Remove
 ↓
Client-side Validation
 ↓
POST /data
```

---

# 5. CSS

## `CSS/style.css`

File CSS dùng chung cho frontend.

Chức năng:

* Styling cho layout của Dashboard.
* Styling cho sidebar.
* Styling cho header/navbar.
* Các style dùng chung cho giao diện.

Ở giai đoạn hiện tại, một số styling riêng của từng chart có thể được đặt trực tiếp trong HTML nếu phù hợp.

---

# 6. JavaScript

## `JavaScript/app.js`

File dành cho các logic ở cấp độ application.

Chức năng:

* Xử lý các hành vi/layout chung của application.
* Chứa các logic không thuộc riêng một page hoặc component cụ thể.

**Chart logic không nên đặt trong file này.**

Chart logic được tách riêng trong:

```text
dashboardChart.js
```

---

## `JavaScript/header.js`

Xử lý component **Header**.

Chức năng:

* Tạo/load Header dùng chung cho các trang frontend.

---

## `JavaScript/navbar.js`

Xử lý component **Navbar**.

Chức năng:

* Tạo/load Navbar dùng chung cho các trang frontend.

---

## `JavaScript/sidebar.js`

Xử lý component **Sidebar**.

Chức năng:

* Tạo/load Sidebar.
* Xác định trạng thái navigation hiện tại.

Ví dụ:

```javascript
loadSidebar("home");
loadSidebar("dataInput");
```

---

## `JavaScript/dashboardChart.js`

Chứa logic liên quan đến các chart trên Dashboard.

Chức năng:

* Gửi request tới C++ backend để lấy dữ liệu.
* Nhận và xử lý JSON.
* Tách `labels` và `values` từ dữ liệu.
* Tạo và cấu hình chart.

Chart hiện tại sử dụng **Chart.js**.

Logic của chart được tách khỏi `app.js` để mỗi file giữ đúng trách nhiệm của nó.

---

## `JavaScript/dataInput.js`

Xử lý logic của trang `DataInput.html`.

Chức năng:

* Đọc input từ người dùng.
* Thực hiện client-side validation.
* Tạo request payload.
* Gửi thao tác Add/Remove thông qua `api.js`.
* Nhận và xử lý response từ backend.
* Hiển thị thông báo thành công/lỗi cho người dùng.

Payload hiện tại:

```json
{
    "category": "C",
    "amount": 50,
    "operation": "add"
}
```

Client-side validation là **lớp validation đầu tiên**.

Backend vẫn là nơi chịu trách nhiệm validation chính thức.

---

## `JavaScript/api.js`

Đây là lớp giao tiếp giữa frontend và backend.

**Lưu ý:** `api.js` không phải là một backend server và cũng không tạo ra một API server mới.

Nhiệm vụ của file này là tập trung các thao tác HTTP giữa JavaScript và C++ backend.

Chức năng hiện tại:

* Gửi HTTP request.
* Chuyển JavaScript object thành JSON.
* Nhận JSON response từ backend.
* Parse response để JavaScript sử dụng.

Endpoint cập nhật hiện tại:

```text
POST http://localhost:8080/data
```

---

# 7. C++ Backend

## `cpp/server.cpp`

File chính của backend/server.

Chức năng:

* Khởi động HTTP server.
* Định nghĩa các API endpoint.
* Lưu trữ dữ liệu hiện tại phía server.
* Parse JSON request.
* Validation request.
* Thực hiện các thao tác Add/Remove.
* Trả JSON response về frontend.

Server hiện tại chạy tại:

```text
http://localhost:8080
```

Các endpoint hiện tại:

```text
GET  /data
POST /data
```

---

### `GET /data`

Dùng để lấy dataset hiện tại từ server.

Ví dụ response:

```json
[
    {"category": "A", "value": 120},
    {"category": "B", "value": 180},
    {"category": "C", "value": 300},
    {"category": "D", "value": 220}
]
```

---

### `POST /data`

Dùng để gửi yêu cầu cập nhật dữ liệu.

Ví dụ request:

```json
{
    "category": "C",
    "amount": 50,
    "operation": "add"
}
```

Các operation hiện tại:

```text
add
remove
```

Backend sẽ validation request trước khi thực sự thay đổi dữ liệu.

Ví dụ:

```text
C = 300

remove 50
→ C = 250

remove 500
→ Reject
→ C vẫn = 300
```

**Một request được server nhận không có nghĩa operation đã được chấp nhận.**

---

## `cpp/httplib.h`

Single-header library của **cpp-httplib**.

Được sử dụng để xây dựng HTTP server trong `server.cpp`.

Được include bằng:

```cpp
#include "httplib.h"
```

---

## `cpp/json.hpp`

Single-header library của **nlohmann/json**.

Được sử dụng để parse và xử lý JSON ở C++ backend.

Được sử dụng bằng:

```cpp
#include "json.hpp"

using json = nlohmann::json;
```

---

# 8. Kiến trúc Validation

Project hiện tại sử dụng **hai lớp validation**.

```text
User Input
    ↓
Client-side Validation
    ↓
POST Request
    ↓
Backend Validation
    ↓
Accept / Reject
```

## Client-side Validation

Được thực hiện trong:

```text
JavaScript/dataInput.js
```

Mục đích là phát hiện những input cơ bản không hợp lệ trước khi gửi request.

Ví dụ:

* Amount không phải số.
* Amount <= 0.
* Category không hợp lệ/không tồn tại ở phía client.

---

## Backend Validation

Được thực hiện trong:

```text
cpp/server.cpp
```

Backend là **nguồn validation có thẩm quyền** đối với dữ liệu.

Ví dụ:

* Amount phải lớn hơn 0.
* Category phải tồn tại.
* Operation phải hợp lệ.
* `remove` không được làm giá trị nhỏ hơn 0.

Do đó:

```text
Client validation PASS
        ↓
Request được gửi
        ↓
Backend validation FAIL
        ↓
Operation bị reject
```

Đây là hành vi bình thường và cần được duy trì khi phát triển hệ thống.

---

# 9. Nguyên tắc phát triển

Project hiện tại được phát triển theo từng bước.

Nguyên tắc ưu tiên:

```text
Understand
    ↓
Build
    ↓
Test
    ↓
Measure
    ↓
Optimize
```

Không nên thêm các optimization hoặc architectural complexity khi chưa có nhu cầu thực tế.

Dataset `A/B/C/D` hiện tại chỉ là **prototype dataset** dùng để kiểm tra architecture và data flow.

Nó **không phải final business data model** của hệ thống.

---

# 10. Định hướng phát triển trong tương lai

Project cuối cùng dự kiến sẽ xử lý dataset lớn hơn đáng kể, có thể bao gồm:

* 1000+ records/elements.
* Nhiều attributes trên mỗi record.
* Các quan hệ dữ liệu phức tạp hơn.
* Warehouse management logic.
* Order management logic.

Khi dataset và business logic phát triển, các vấn đề có thể cần xem xét:

* Efficient data retrieval.
* Partial update.
* Filtering và querying.
* Pagination.
* Caching.
* Data aggregation.
* Frontend rendering performance.

Các vấn đề trên là **future considerations**, chưa phải yêu cầu của prototype hiện tại.

Không nên tối ưu chúng trước khi có requirement hoặc measurement thực tế.

---

# 11. Quy tắc phân chia trách nhiệm

Khi phát triển thêm project, nên duy trì separation of responsibilities:

```text
HTML
    → Cấu trúc trang

CSS
    → Giao diện và styling

JavaScript
    → Frontend behavior

api.js
    → Frontend ↔ Backend communication

server.cpp
    → Backend logic + validation + data processing
```

Ví dụ:

* Không đưa business logic của C++ vào JavaScript nếu không cần thiết.
* Không đưa chart logic vào `app.js`.
* Không biến `api.js` thành một server mới.
* Backend vẫn phải tự validation request, không phụ thuộc hoàn toàn vào client-side validation.

---

# 12. Quy tắc dành cho việc tiếp tục phát triển

README này được dùng như tài liệu đồng bộ kiến trúc cho các thành viên và các agent khác tham gia project.

Trước khi thực hiện một thay đổi lớn:

1. Đọc và hiểu data flow hiện tại.
2. Xác định component nào đang chịu trách nhiệm cho phần cần sửa.
3. Ưu tiên thay đổi nhỏ nhất cần thiết.
4. Test toàn bộ flow:

```text
Frontend
    ↓
API
    ↓
C++ Backend
    ↓
Response
    ↓
Frontend
```

5. Chỉ refactor hoặc optimize sau khi functionality hiện tại đã hoạt động ổn định.

Nếu một thay đổi làm thay đổi architecture hoặc responsibility của các component, README này cần được cập nhật.

---

# 13. Trạng thái hiện tại

## Đã hoàn thành

* [x] Basic Dashboard HTML layout
* [x] Header
* [x] Navbar
* [x] Sidebar
* [x] Dashboard chart prototype
* [x] C++ HTTP server
* [x] JSON parsing
* [x] `GET /data`
* [x] `POST /data`
* [x] Add operation
* [x] Remove operation
* [x] Client-side validation
* [x] Backend validation
* [x] Frontend xử lý success/error response
* [x] Frontend ↔ C++ communication

## Chưa triển khai

* [ ] Final Warehouse data model
* [ ] Final Order data model
* [ ] Warehouse management logic
* [ ] Order management logic
* [ ] Persistent database/storage
* [ ] Authentication/authorization
* [ ] Production deployment
* [ ] Large-scale performance optimization

---

# 14. Giai đoạn phát triển hiện tại

Project hiện đang ở giai đoạn:

> **Technical Foundation / Prototype**

Mục tiêu hiện tại là xây dựng và kiểm chứng một pipeline frontend/backend rõ ràng và hoạt động ổn định:

```text
HTML Dashboard
      ↕
JavaScript
      ↕
HTTP / JSON API
      ↕
C++ Backend
      ↕
Server-side Data
```

Business requirements và final data model sẽ được xác định ở các giai đoạn phát triển tiếp theo.
