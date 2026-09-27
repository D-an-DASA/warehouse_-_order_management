#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

// ==========================================
// 1. CẤU TRÚC DỮ LIỆU SẢN PHẨM
// ==========================================

struct Product {
    string id;
    string name;
    double price;
};

// ==========================================
// 2. DSA CORE: HASH TABLE (FROM SCRATCH)
// ==========================================

class HashTable {
private:
    struct Node {
        string key;
        Product value;
        Node(string k, Product v) : key(k), value(v) {}
    };

    vector<list<Node>> buckets;
    int capacity;

    // Hàm băm DJB2: Chuyển string ID thành index
    int hashFunction(const string& key) {
        unsigned long hash = 5381;
        for (char c : key) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash % capacity;
    }

public:
    HashTable(int cap = 1000) : capacity(cap) {
        buckets.resize(capacity);
    }

    // Thêm sản phẩm vào bảng
    void insert(const string& key, const Product& value) {
        int index = hashFunction(key);
        for (auto& node : buckets[index]) {
            if (node.key == key) {
                node.value = value;
                return;
            }
        }
        buckets[index].push_back(Node(key, value));
    }

    // Tìm kiếm sản phẩm theo ID - Trả về con trỏ (nullptr nếu không có)
    Product* search(const string& key) {
        int index = hashFunction(key);
        for (auto& node : buckets[index]) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr;
    }
};

// ==========================================
// 3. ĐỌC FILE CSV
// ==========================================

void loadProductsFromCSV(const string& filename, HashTable& table) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Loi: Khong the mo file " << filename << endl;
        return;
    }

    string line;
    // Bỏ qua dòng tiêu đề (header)
    getline(file, line);

    int count = 0;
    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string id, name, priceStr;

        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, priceStr, ',');

        if (!id.empty()) {
            Product p;
            p.id = id;
            p.name = name;
            try {
                p.price = stod(priceStr);
            } catch (...) {
                p.price = 0.0;
            }
            table.insert(p.id, p);
            count++;
        }
    }
    file.close();
    cout << "Da load " << count << " san pham tu file " << filename << endl;
}

// ==========================================
// 4. MAIN - TÌM KIẾM SẢN PHẨM THEO ID
// ==========================================

int main() {
    // Khởi tạo Hash Table
    HashTable productTable(1000);

    // Đọc dữ liệu từ file CSV
    loadProductsFromCSV("products.csv", productTable);

    // Nhập ID cần tìm
    string searchId;
    cout << "\nNhap ID san pham can tim: ";
    cin >> searchId;

    // Tìm kiếm trong Hash Table (O(1))
    Product* result = productTable.search(searchId);

    if (result != nullptr) {
        cout << "\n[CO] San pham ton tai!" << endl;
        cout << "  ID   : " << result->id << endl;
        cout << "  Ten  : " << result->name << endl;
        cout << "  Gia  : " << result->price << endl;
    } else {
        cout << "\n[KHONG] San pham voi ID \"" << searchId << "\" khong ton tai!" << endl;
    }

    return 0;
}