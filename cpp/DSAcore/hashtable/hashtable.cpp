// ==========================================================
// hastable.cpp - Cài đặt Hash Table tra cứu sản phẩm theo ID
// ==========================================================
#include "hashtable.h"

// ==========================================================
// Constructor
// ==========================================================
HashTable::HashTable() {
    capacity = 128;              // Bắt đầu với 128 bucket
    size = 0;                    // Chưa có sản phẩm nào
    buckets.resize(capacity);    // Khởi tạo mảng bucket rỗng
}

// ==========================================================
// Destructor
// ==========================================================
HashTable::~HashTable() {
    // vector + list tự động giải phóng bộ nhớ
    // Không cần làm gì thêm
}

// ==========================================================
// Hàm băm: chuyển chuỗi ID thành index 0..cap-1
// Công thức: hash = hash * 31 + ASCII(char) (giống Java)
// ==========================================================
int HashTable::hashFunc(const string& id, int cap) const {
    long long hashValue = 0;

    for (char c : id) {
        hashValue = (hashValue * 31 + c) % cap;
    }

    return hashValue;
}

// ==========================================================
// Mở rộng bảng khi load factor vượt 0.75
// ==========================================================
void HashTable::resize() {
    int newCap = capacity * 2;                     // Capacity mới = gấp đôi
    vector<list<Product>> newBuckets(newCap);      // Tạo mảng bucket mới

    // Duyệt toàn bộ sản phẩm cũ, băm lại sang bảng mới
    for (int i = 0; i < capacity; i++) {
        for (auto& p : buckets[i]) {
            int newIdx = hashFunc(p.id, newCap);
            newBuckets[newIdx].push_back(p);
        }
    }

    buckets = newBuckets;   // Thay bảng cũ bằng bảng mới
    capacity = newCap;      // Cập nhật capacity
}

// ==========================================================
// Thêm 1 sản phẩm vào bảng
// ==========================================================
bool HashTable::insert(const Product& p) {

    int idx = hashFunc(p.id, capacity);

    // Mỗi ID chỉ đại diện cho một sản phẩm.
    // Từ chối ID trùng để không ghi đè sản phẩm đang lưu.
    for (const auto& current : buckets[idx]) {
        if (current.id == p.id) {
            return false;
        }
    }
    
    // Kiểm tra load factor trước khi thêm
    // (size+1)/capacity > 0.75  <=>  (size+1)*4 > capacity*3
    if ((size + 1) * 4 > capacity * 3) {
        resize();
        idx = hashFunc(p.id, capacity);
    }

    buckets[idx].push_back(p);            // Thêm vào bucket
    size++;                               // Tăng số đếm
    return true;
}

// ==========================================================
// Tìm sản phẩm theo ID
// Trả về con trỏ tới Product (nullptr nếu không có)
// ==========================================================
Product* HashTable::search(const string& id) {
    int idx = hashFunc(id, capacity);      // Tính index cần tìm

    // Chỉ duyệt bucket đó (O(1) trung bình)
    for (auto& p : buckets[idx]) {
        if (p.id == id) return &p;         // Tìm thấy -> trả về địa chỉ
    }
    return nullptr;                        // Không tìm thấy
}

const Product* HashTable::search(const string& id) const {
    int idx = hashFunc(id, capacity);

    for (const auto& p : buckets[idx]) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

// ==========================================================
// Xóa sản phẩm theo ID
// Trả về true nếu xóa thành công, false nếu không tìm thấy
// ==========================================================
bool HashTable::remove(const string& id) {
    int idx = hashFunc(id, capacity);
    auto& bucket = buckets[idx];

    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->id == id) {
            bucket.erase(it);   // Xóa node khỏi list
            size--;             // Giảm số đếm
            return true;
        }
    }
    return false;   // Không tìm thấy để xóa
}

vector<Product> HashTable::values() const {
    vector<Product> result;
    result.reserve(static_cast<size_t>(size));

    for (const auto& bucket : buckets) {
        for (const auto& product : bucket) {
            result.push_back(product);
        }
    }
    return result;
}
