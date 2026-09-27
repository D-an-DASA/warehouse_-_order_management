// ==========================================================
// KHAI BÁO THƯ VIỆN
// ==========================================================
#include <iostream>      // cin, cout - nhập/xuất màn hình
#include <vector>        // vector - mảng động
#include <list>          // list - danh sách liên kết đôi
#include <string>        // string - xử lý chuỗi
#include <fstream>       // ifstream - đọc file
#include <sstream>       // stringstream - tách chuỗi
#include <filesystem>    // directory_iterator - duyệt thư mục (C++17)

using namespace std;             // Cho phép viết cout thay vì std::cout
namespace fs = std::filesystem;  // Bí danh "fs" cho std::filesystem

// ==========================================================
// CẤU TRÚC SẢN PHẨM (6 trường khớp với 6 cột CSV)
// ==========================================================
struct Product {
    string id;           // Cột 1: Mã sản phẩm (P00001)
    string name;         // Cột 2: Tên sản phẩm (Power Bank)
    string madeDate;     // Cột 3: Ngày sản xuất (2023-02-21)
    string arrivedTime;  // Cột 4: Ngày đến kho (2023-03-17 08:15:14)
    string bestByDate;   // Cột 5: Hạn sử dụng (2024-01-02)
    string status;       // Cột 6: Trạng thái (EXPIRED/AVAILABLE/RESERVED)
};

// ==========================================================
// HASH TABLE TỰ ĐỘNG MỞ RỘNG
// ==========================================================
class HashTable {
private:
    // Mảng các bucket - mỗi bucket là 1 list chứa nhiều Product
    vector<list<Product>> buckets;

    int capacity;    // Số bucket hiện tại (bắt đầu 128, tự nhân đôi khi cần)
    int size;        // Tổng số sản phẩm đã thêm vào bảng

    // ------------------------------------------------------
    // Hàm băm: chuyển chuỗi ID thành index trong khoảng 0..cap-1
    // ------------------------------------------------------
    int hashFunc(const string& id, int cap) {
        int sum = 0;                               // Biến tích lũy, bắt đầu = 0
        for (int i = 0; i < id.length(); i++) {    // Duyệt từng ký tự của chuỗi ID
            sum = sum * 31 + id[i];                // Nhân 31 + mã ASCII của ký tự
        }
        if (sum < 0) sum = -sum;                   // Nếu bị âm (tràn số) thì đổi dấu
        return sum % cap;                          // Chia lấy dư -> index 0..cap-1
    }

    // ------------------------------------------------------
    // Mở rộng bảng khi load factor vượt 0.75
    // ------------------------------------------------------
    void resize() {
        int newCap = capacity * 2;                     // Capacity mới = gấp đôi
        vector<list<Product>> newBuckets(newCap);      // Tạo mảng bucket mới

        for (int i = 0; i < capacity; i++) {           // Duyệt từng bucket cũ
            for (auto& p : buckets[i]) {               // Duyệt từng Product trong bucket
                int newIdx = hashFunc(p.id, newCap);   // Băm lại theo capacity mới
                newBuckets[newIdx].push_back(p);       // Đưa Product vào bucket mới
            }
        }

        buckets = newBuckets;   // Thay bảng cũ bằng bảng mới
        capacity = newCap;      // Cập nhật capacity
    }

public:
    // ------------------------------------------------------
    // Constructor: tự động chạy khi tạo đối tượng HashTable
    // ------------------------------------------------------
    HashTable() {
        capacity = 128;              // Bắt đầu với 128 bucket (nhỏ, tiết kiệm RAM)
        size = 0;                    // Chưa có sản phẩm nào
        buckets.resize(capacity);    // Khởi tạo mảng 128 bucket rỗng
    }

    // ------------------------------------------------------
    // Thêm 1 sản phẩm vào bảng
    // ------------------------------------------------------
    void insert(Product p) {
        // Kiểm tra load factor trước khi thêm.
        // (size+1)/capacity > 0.75 tương đương (size+1)*4 > capacity*3
        if ((size + 1) * 4 > capacity * 3) {
            resize();                // Nếu sắp đầy -> mở rộng trước
        }

        int idx = hashFunc(p.id, capacity);   // Tính index của sản phẩm
        buckets[idx].push_back(p);            // Thêm Product vào cuối bucket
        size++;                               // Tăng số đếm
    }

    // ------------------------------------------------------
    // Tìm kiếm sản phẩm theo ID
    // Trả về con trỏ Product (nullptr nếu không có)
    // ------------------------------------------------------
    Product* search(string id) {
        int idx = hashFunc(id, capacity);      // Tính index cho ID cần tìm

        for (auto& p : buckets[idx]) {         // Chỉ duyệt bucket đó (O(1))
            if (p.id == id) return &p;         // Nếu khớp ID -> trả về địa chỉ
        }
        return nullptr;                        // Duyệt hết mà không có -> nullptr
    }
};

// ==========================================================
// ĐỌC FILE CSV (6 cột)
// Trả về số sản phẩm đã load
// ==========================================================
int loadFromCSV(string filename, HashTable& table) {
    ifstream file(filename);              // Mở file để đọc
    if (!file.is_open()) return 0;        // Không mở được -> trả về 0

    string line;
    getline(file, line);                  // Đọc dòng đầu (header) và bỏ qua

    int count = 0;                        // Biến đếm số sản phẩm đã load

    while (getline(file, line)) {         // Đọc từng dòng còn lại
        if (line.empty()) continue;       // Bỏ qua dòng trống

        stringstream ss(line);            // Tạo stream từ dòng vừa đọc
        string id, name, madeDate, arrivedTime, bestByDate, status;

        // Tách dòng theo dấu phẩy -> 6 cột
        getline(ss, id, ',');             // Cột 1: ID
        getline(ss, name, ',');           // Cột 2: Tên sản phẩm
        getline(ss, madeDate, ',');       // Cột 3: Ngày sản xuất
        getline(ss, arrivedTime, ',');    // Cột 4: Ngày đến kho
        getline(ss, bestByDate, ',');     // Cột 5: Hạn sử dụng
        getline(ss, status, ',');         // Cột 6: Trạng thái

        if (!id.empty()) {                // Nếu ID không rỗng -> dòng hợp lệ
            Product p;
            p.id = id;                    // Gán từng trường
            p.name = name;
            p.madeDate = madeDate;
            p.arrivedTime = arrivedTime;
            p.bestByDate = bestByDate;
            p.status = status;

            table.insert(p);              // Thêm vào Hash Table
            count++;                      // Tăng biến đếm
        }
    }
    file.close();                         // Đóng file
    return count;                         // Trả về số sản phẩm đã load
}

// ==========================================================
// MAIN - TỰ ĐỘNG QUÉT MỌI FILE CSV TRONG THƯ MỤC
// ==========================================================
int main() {
    HashTable productTable;   // Tạo Hash Table rỗng (capacity = 128)

    int totalLoaded = 0;      // Tổng số sản phẩm đã load
    int fileCount = 0;        // Số file CSV đã xử lý

    cout << "=== TU DONG TIM FILE CSV ===" << endl;

    // Duyệt mọi file trong thư mục hiện tại (".")
    for (const auto& entry : fs::directory_iterator(".")) {
        if (!entry.is_regular_file()) continue;   // Bỏ qua nếu không phải file thường

        string name = entry.path().filename().string();  // Lấy tên file (không có path)

        // Kiểm tra file có đuôi .csv (4 ký tự cuối == ".csv")
        if (name.size() >= 4 && name.substr(name.size() - 4) == ".csv") {
            cout << "Dang load: " << name << " ... ";

            int n = loadFromCSV(name, productTable);   // Gọi hàm load
            cout << n << " san pham" << endl;          // In số SP đã load

            totalLoaded += n;   // Cộng dồn vào tổng
            fileCount++;        // Tăng số file
        }
    }

    // Nếu không có file CSV nào trong thư mục
    if (fileCount == 0) {
        cout << "Loi: Khong tim thay file CSV nao!" << endl;
        return 1;           // Thoát với mã lỗi
    }

    // In tổng kết
    cout << "\nTong cong: " << totalLoaded << " san pham tu "
         << fileCount << " file" << endl;

    // Nhập ID cần tìm
    string searchId;
    cout << "\nNhap ID san pham can tim: ";
    cin >> searchId;        // Đọc ID từ bàn phím

    // Gọi hàm tìm kiếm (O(1))
    Product* result = productTable.search(searchId);

    // In kết quả
    if (result != nullptr) {   // Nếu tìm thấy
        cout << "\n[CO] San pham ton tai!" << endl;
        cout << "  ID          : " << result->id << endl;
        cout << "  Ten         : " << result->name << endl;
        cout << "  Ngay SX     : " << result->madeDate << endl;
        cout << "  Ngay den    : " << result->arrivedTime << endl;
        cout << "  Han su dung : " << result->bestByDate << endl;
        cout << "  Trang thai  : " << result->status << endl;
    } else {                    // Nếu không tìm thấy
        cout << "\n[KHONG] San pham voi ID \"" << searchId
             << "\" khong ton tai!" << endl;
    }

    return 0;   // Kết thúc thành công
}
