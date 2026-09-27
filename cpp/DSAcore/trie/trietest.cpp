#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include "trie.h"
#include "../Product.h"

using namespace std;
using namespace std::chrono;

// ============================================================
// ĐỌC CSV
// ============================================================
vector<Product> loadProducts(const string& filename) {
    vector<Product> products;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "[ERROR] Khong mo duoc file: " << filename << endl;
        return products;
    }
    string line;
    getline(file, line); // bỏ header
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Product p;
        getline(ss, p.id, ',');
        getline(ss, p.product_name, ',');
        getline(ss, p.made_date, ',');
        getline(ss, p.arrived_time, ',');
        getline(ss, p.best_by_date, ',');
        getline(ss, p.status, ',');
        products.push_back(p);
    }
    file.close();
    return products;
}

// ============================================================
// TEST 1: INSERT + SEARCH với dữ liệu thật
// ============================================================
void testWithRealNames() {
    cout << "\n=== Test 1: Insert + Search voi ten that ===" << endl;
    Trie trie;
    trie.insert("Power Bank", "P001");
    trie.insert("USB Cable", "P002");
    trie.insert("Laptop Stand", "P003");
    trie.insert("USB Hub", "P004");

    // Search "USB" - phải tìm được 2 sản phẩm
    auto r = trie.searchByPrefix("USB");
    cout << "  'USB': " << r.size() << " ket qua (mong doi 2)" << endl;
    assert(r.size() == 2);

    // Search "Power" - phải tìm được 1
    r = trie.searchByPrefix("Power");
    cout << "  'Power': " << r.size() << " ket qua (mong doi 1)" << endl;
    assert(r.size() == 1);

    // Search "Lap" - phải tìm được 1
    r = trie.searchByPrefix("Lap");
    cout << "  'Lap': " << r.size() << " ket qua (mong doi 1)" << endl;
    assert(r.size() == 1);

    // Search "XYZ" - không có
    r = trie.searchByPrefix("XYZ");
    assert(r.empty());
    cout << "  'XYZ': 0 ket qua" << endl;

    cout << "  => Test 1 PASSED!" << endl;
}

// ============================================================
// TEST 2: REMOVE
// ============================================================
void testRemove() {
    cout << "\n=== Test 2: Remove ===" << endl;
    Trie trie;
    trie.insert("Power Bank", "P001");
    trie.insert("Power Adapter", "P002");

    assert(trie.remove("Power Bank", "P001") == true);
    auto r = trie.searchByPrefix("Power");
    assert(r.size() == 1 && r[0] == "P002");
    cout << "  Xoa P001 thanh cong" << endl;

    assert(trie.remove("XYZ", "P999") == false);
    cout << "  Xoa khong ton tai -> OK" << endl;
    cout << "  => Test 2 PASSED!" << endl;
}

// ============================================================
// TEST 3: EDGE CASES
// ============================================================
void testEdgeCases() {
    cout << "\n=== Test 3: Edge Cases ===" << endl;

    Trie emptyTrie;
    assert(emptyTrie.searchByPrefix("any").empty());
    cout << "  Trie rong -> OK" << endl;

    Trie trie;
    trie.insert("USB Cable", "P001");
    trie.insert("Power Bank", "P002");
    auto r = trie.searchByPrefix("");
    assert(r.size() == 2);
    cout << "  Tim '': " << r.size() << " ket qua" << endl;

    r = trie.searchByPrefix("USB Cable Pro Max Ultra");
    assert(r.empty());
    cout << "  Prefix dai hon ten -> OK" << endl;

    Trie trie2;
    trie2.insert("USB Cable", "P001");
    trie2.insert("USB Cable", "P002");
    r = trie2.searchByPrefix("USB Cable");
    assert(r.size() == 2);
    cout << "  Nhieu san pham cung ten: " << r.size() << " ket qua" << endl;
    cout << "  => Test 3 PASSED!" << endl;
}

// ============================================================
// BENCHMARK VỚI DATASET
// ============================================================
void benchmark(const string& filename, int expected) {
    cout << "\n========================================" << endl;
    cout << "BENCHMARK: " << filename << endl;
    cout << "========================================" << endl;

    auto t0 = high_resolution_clock::now();
    auto products = loadProducts(filename);
    auto t1 = high_resolution_clock::now();
    auto loadMs = duration_cast<milliseconds>(t1 - t0).count();

    if (products.empty()) {
        cout << "  [SKIP] Khong doc duoc du lieu." << endl;
        return;
    }

    cout << "  So san pham: " << products.size()
         << " (mong doi " << expected << ")" << endl;
    cout << "  Doc CSV: " << loadMs << " ms" << endl;

    // Build Trie
    Trie trie;
    t0 = high_resolution_clock::now();
    for (const auto& p : products) {
        trie.insert(p.product_name, p.id);
    }
    t1 = high_resolution_clock::now();
    auto buildMs = duration_cast<milliseconds>(t1 - t0).count();
    cout << "  [Trie] Build: " << buildMs << " ms" << endl;

    // Test prefix thật trong dataset
    vector<string> prefixes = {"Power", "USB", "Lap", "Green", "Samp"};
    int iter = 1000;

    t0 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++)
        for (auto& p : prefixes)
            trie.searchByPrefix(p);
    t1 = high_resolution_clock::now();
    auto trieUs = duration_cast<microseconds>(t1 - t0).count();
    cout << "  [Trie] Search (" << iter * prefixes.size() << " lan): "
         << trieUs << " us" << endl;

    // Kiểm tra kết quả thực tế
    cout << "\n  Vi du ket qua tra cuu:" << endl;
    for (auto& p : prefixes) {
        auto r = trie.searchByPrefix(p);
        cout << "    '" << p << "': " << r.size() << " san pham" << endl;
    }

    // Linear scan (baseline)
    t0 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++) {
        for (auto& prefix : prefixes) {
            vector<string> res;
            for (const auto& p : products) {
                if (p.product_name.substr(0, prefix.size()) == prefix)
                    res.push_back(p.id);
            }
        }
    }
    t1 = high_resolution_clock::now();
    auto linearUs = duration_cast<microseconds>(t1 - t0).count();
    cout << "\n  [Linear] Search (" << iter * prefixes.size() << " lan): "
         << linearUs << " us" << endl;

    if (trieUs > 0)
        cout << "  ==> Trie nhanh hon " << (double)linearUs / trieUs
             << " lan" << endl;
}

// ============================================================
// MAIN
// ============================================================
int main() {
    cout << "========================================" << endl;
    cout << "   TEST & BENCHMARK TRIE - YC 4.2" << endl;
    cout << "   Nguoi thuc hien: Duong Minh Phuc" << endl;
    cout << "========================================" << endl;

    testWithRealNames();
    testRemove();
    testEdgeCases();

    // Đường dẫn từ DSAcore/trie/ đến cpp/data/
//benchmark("../../product_inventory_10 000.csv", 10000);
benchmark("../../product_inventory_100 000.csv", 100000);

    cout << "\n========================================" << endl;
    cout << "TAT CA TEST TRIE DA PASS!" << endl;
    cout << "========================================" << endl;
    return 0;
}
