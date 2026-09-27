// ==========================================
// KHAI BÁO THƯ VIỆN
// ==========================================
#include <iostream>      // cin, cout - nhập xuất cơ bản
#include <vector>        // vector - mảng động
#include <list>          // list - danh sách liên kết đôi
#include <string>        // string - xử lý chuỗi
#include <fstream>       // ifstream - đọc file
#include <sstream>       // stringstream - tách chuỗi
#include <filesystem>    // directory_iterator - duyệt thư mục (C++17)

using namespace std;             // Cho phép viết cout thay vì std::cout
namespace fs = std::filesystem;  // Bí danh fs cho std::filesystem

// ==========================================
// CẤU TRÚC SẢN PHẨM
// ==========================================
struct Product {
    string id;       // Mã sản phẩm
    string name;     // Tên sản phẩm
    double price;    // Giá sản phẩm
};

// ==========================================
// HASH TABLE TỰ ĐỘNG MỞ RỘNG
// ==========================================
class HashTable {
private:
    // Mảng các bucket - mỗi bucket là 1 danh sách liên kết chứa Product
    vector<list<Product>> buckets;
    
    int capacity;    // Số bucket hiện tại (bắt đầu 128, tăng gấp đôi khi cần)
    int size;        // Tổng số sản phẩm đã thêm vào bảng

    // -----------------------------------------
    // Hàm băm: chuyển chuỗi ID thành index 0..cap-1
    // -----------------------------------------
    int hashFunc(const string& id, int cap) {
        int sum = 0;                              // Biến tích lũy
        for (int i = 0; i < id.length(); i++) {   // Duyệt từng ký tự
            sum = sum * 31 + id[i];               // Nhân 31 + mã ASCII (giống Java)
        }
        if (sum < 0) sum = -sum;                  // Tránh số âm (do tràn số)
        return sum % cap;                         // Chia lấy dư -> index 0..cap-1
    }

    // -----------------------------------------
    // Mở rộng bảng khi load factor > 0.75
    // -----------------------------------------
    void resize() {
        int newCap = capacity * 2;                          // Gấp đôi capacity
        vector<list<Product>> newBuckets(newCap);           // Tạo bảng mới

        for (int i = 0; i < capacity; i++) {                // Duyệt bucket cũ
            for (auto& p : buckets[i]) {                    // Duyệt từng sản phẩm
                int newIdx = hashFunc(p.id, newCap);        // Băm lại theo cap mới
                newBuckets[newIdx].push_back(p);            // Đưa vào bảng mới
            }
        }

        buckets = newBuckets;    // Thay bảng cũ bằng bảng mới
        capacity = newCap;       // Cập nhật capacity
    }

public:
    // -----------------------------------------
    // Constructor: khởi tạo bảng rỗng 128 bucket
    // -----------------------------------------
    HashTable() {
        capacity = 128;              // Bắt đầu nhỏ để tiết kiệm RAM
        size = 0;                    // Chưa có sản phẩm nào
        buckets.resize(capacity);    // Khởi tạo mảng 128 bucket rỗng
    }

    // -----------------------------------------
    // Thêm 1 sản phẩm vào bảng
    // -----------------------------------------
    void insert(Product p) {
        // Kiểm tra load factor trước khi thêm: nếu sắp > 0.75 thì resize
        // (size+1)*4 > capacity*3  <=>  (size+1)/capacity > 0.75
        if ((size + 1) * 4 > capacity * 3) {
            resize();
        }

        int idx = hashFunc(p.id, capacity);   // Tính index cho sản phẩm
        buckets[idx].push_back(p);            // Thêm vào cuối bucket
        size++;                               // Tăng số đếm
    }

    // -----------------------------------------
    // Tìm kiếm sản phẩm theo ID
    // Trả về con trỏ tới Product, hoặc nullptr nếu không có
    // -----------------------------------------
    Product* search(string id) {
        int idx = hashFunc(id, capacity);     // Tính index cần tìm

        for (auto& p : buckets[idx]) {        // Chỉ duyệt bucket đó (O(1))
            if (p.id == id) return &p;        // Tìm thấy -> trả về địa chỉ
        }
        return nullptr;                       // Không tìm thấy
    }
};

// ==========================================
// ĐỌC FILE CSV
// Trả về số sản phẩm đã load được
// ==========================================
int loadFromCSV(string filename, HashTable& table) {
    ifstream file(filename);                  // Mở file để đọc
    if (!file.is_open()) return 0;            // Không mở được -> trả về 0

    string line;
    getline(file, line);                      // Bỏ dòng header (id,name,price)

    int count = 0;                            // Đếm số sản phẩm đã load

    while (getline(file, line)) {             // Đọc từng dòng
        if (line.empty()) continue;           // Bỏ qua dòng trống

        stringstream ss(line);                // Tạo stream từ dòng vừa đọc
        string id, name, priceStr;            // 3 biến tạm cho 3 cột

        getline(ss, id, ',');                 // Đọc đến dấu phẩy 1 -> id
        getline(ss, name, ',');               // Đọc đến dấu phẩy 2 -> name
        getline(ss, priceStr, ',');           // Đọc phần còn lại -> giá (chuỗi)

        if (!id.empty()) {                    // Dòng hợp lệ
            Product p;
            p.id = id;
            p.name = name;

            // Chuyển chuỗi giá thành số double, nếu lỗi thì gán 0
            try {
                p.price = stod(priceStr);
            } catch (...) {
                p.price = 0;
            }

            table.insert(p);                  // Thêm vào Hash Table
            count++;                          // Tăng bộ đếm
        }
    }
    file.close();                             // Đóng file
    return count;                             // Trả về số SP đã load
}

// ==========================================
// MAIN - TỰ ĐỘNG QUÉT MỌI FILE CSV
// ==========================================
int main() {
    HashTable productTable;      // Tạo Hash Table rỗng (capacity = 128)
    int totalLoaded = 0;         // Tổng số sản phẩm đã load
    int fileCount = 0;           // Số file CSV đã xử lý

    cout << "=== TU DONG TIM FILE CSV ===" << endl;

    // Duyệt mọi file trong thư mục hiện tại (".")
    for (const auto& entry : fs::directory_iterator(".")) {
        if (!entry.is_regular_file()) continue;   // Bỏ qua thư mục con

        string name = entry.path().filename().string();  // Lấy tên file

        // Kiểm tra đuôi .csv (4 ký tự cuối == ".csv")
        if (name.size() >= 4 && name.substr(name.size() - 4) == ".csv") {
            cout << "Dang load: " << name << " ... ";

            int n = loadFromCSV(name, productTable);   // Load file
            cout << n << " san pham" << endl;

            totalLoaded += n;    // Cộng dồn
            fileCount++;         // Tăng số file
        }
    }

    // Nếu không tìm thấy file CSV nào
    if (fileCount == 0) {
        cout << "Loi: Khong tim thay file CSV nao trong thu muc!" << endl;
        return 1;
    }

    // In tổng kết
    cout << "\nTong cong: " << totalLoaded << " san pham tu " 
         << fileCount << " file" << endl;

    // Nhập ID cần tìm
    string searchId;
    cout << "\nNhap ID san pham can tim: ";
    cin >> searchId;

    // Tìm kiếm trong Hash Table (O(1))
    Product* result = productTable.search(searchId);

    // In kết quả
    if (result != nullptr) {                  // Tìm thấy
        cout << "\n[CO] San pham ton tai!" << endl;
        cout << "  ID  : " << result->id << endl;
        cout << "  Ten : " << result->name << endl;
        cout << "  Gia : " << result->price << endl;
    } else {                                  // Không tìm thấy
        cout << "\n[KHONG] San pham voi ID \"" << searchId 
             << "\" khong ton tai!" << endl;
    }

    return 0;    // Kết thúc thành công
}
