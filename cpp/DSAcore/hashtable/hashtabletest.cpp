// ==========================================================
// hastabletest.cpp - Test & Benchmark cho HashTable
// ==========================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include "hashtable.h"
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
    getline(file, line);   // bỏ header
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
// TEST 1: INSERT + SEARCH
// ============================================================
void testInsertSearch() {
    cout << "\n=== Test 1: Insert + Search ===" << endl;
    HashTable ht;

    ht.insert({"P001", "Power Bank", "2023-02-21", "2023-03-17 08:15:14", "2024-01-02", "EXPIRED"});
    ht.insert({"P002", "USB Cable", "2023-03-23", "2023-03-31 16:38:01", "2025-06-03", "EXPIRED"});
    ht.insert({"P003", "Laptop Stand", "2025-05-15", "2025-05-26 08:09:13", "2027-05-04", "AVAILABLE"});

    Product* r = ht.search("P001");
    assert(r != nullptr && r->product_name == "Power Bank");
    cout << "  Tim P001: " << r->product_name << " -> OK" << endl;

    r = ht.search("P003");
    assert(r != nullptr && r->status == "AVAILABLE");
    cout << "  Tim P003: " << r->status << " -> OK" << endl;

    r = ht.search("P999");
    assert(r == nullptr);
    cout << "  Tim P999: khong ton tai -> OK" << endl;

    cout << "  => Test 1 PASSED!" << endl;
}

// ============================================================
// TEST 2: REMOVE
// ============================================================
void testRemove() {
    cout << "\n=== Test 2: Remove ===" << endl;
    HashTable ht;
    ht.insert({"P001", "Power Bank", "", "", "", "EXPIRED"});
    ht.insert({"P002", "USB Cable", "", "", "", "EXPIRED"});

    assert(ht.remove("P001") == true);
    assert(ht.search("P001") == nullptr);
    assert(ht.search("P002") != nullptr);
    cout << "  Xoa P001 -> OK" << endl;

    assert(ht.remove("P999") == false);
    cout << "  Xoa P999 (khong ton tai) -> OK" << endl;

    cout << "  => Test 2 PASSED!" << endl;
}

// ============================================================
// TEST 3: EDGE CASES
// ============================================================
void testEdgeCases() {
    cout << "\n=== Test 3: Edge Cases ===" << endl;

    HashTable emptyHt;
    assert(emptyHt.search("any") == nullptr);
    cout << "  Bang rong -> OK" << endl;

    HashTable ht;
    ht.insert({"P001", "Power Bank", "", "", "", "EXPIRED"});
    assert(ht.search("") == nullptr);
    cout << "  Tim ID rong -> OK" << endl;

    ht.insert({"P001", "Power Bank v2", "", "", "", "EXPIRED"});
    Product* r = ht.search("P001");
    cout << "  Insert trung ID: " << r->product_name << endl;

    HashTable ht2;
    ht2.insert({"P100", "Old Milk", "", "", "2020-01-01", "EXPIRED"});
    Product* expired = ht2.search("P100");
    assert(expired != nullptr && expired->status == "EXPIRED");
    cout << "  San pham het han: " << expired->status << " -> OK" << endl;

    cout << "  => Test 3 PASSED!" << endl;
}

// ============================================================
// BENCHMARK (KHÔNG DÙNG LINEAR SCAN)
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

    HashTable ht;
    t0 = high_resolution_clock::now();
    for (const auto& p : products) {
        ht.insert(p);
    }
    t1 = high_resolution_clock::now();
    auto buildMs = duration_cast<milliseconds>(t1 - t0).count();

    cout << "  [HashTable] Build: " << buildMs << " ms" << endl;
    cout << "  [HashTable] Capacity cuoi: " << ht.getCapacity() << " buckets" << endl;
    cout << "  [HashTable] Load factor: "
         << (double)products.size() / ht.getCapacity() << endl;

    vector<string> ids;
    ids.push_back(products.front().id);
    ids.push_back(products[100].id);
    ids.push_back(products[products.size() / 2].id);
    ids.push_back(products.back().id);

    int iter = 1000;

    t0 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++) {
        for (auto& id : ids) {
            ht.search(id);
        }
    }
    t1 = high_resolution_clock::now();
    auto hashUs = duration_cast<microseconds>(t1 - t0).count();

    cout << "  [HashTable] Search (" << iter * ids.size() << " lan): "
         << hashUs << " us" << endl;
    cout << "  [HashTable] Trung binh: "
         << (double)hashUs / (iter * ids.size()) << " us/lan" << endl;

    cout << "\n  Vi du ket qua tra cuu:" << endl;
    for (auto& id : ids) {
        Product* r = ht.search(id);
        if (r) {
            cout << "    " << id << " -> " << r->product_name
                 << " (" << r->status << ")" << endl;
        }
    }

    t0 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++) {
        ht.search("NOT_EXIST_" + to_string(i));
    }
    t1 = high_resolution_clock::now();
    auto missUs = duration_cast<microseconds>(t1 - t0).count();
    cout << "\n  [HashTable] Search ID khong ton tai (" << iter << " lan): "
         << missUs << " us" << endl;
}

// ============================================================
// MAIN
// ============================================================
int main() {
    cout << "========================================" << endl;
    cout << "   TEST & BENCHMARK HASHTABLE" << endl;
    cout << "========================================" << endl;
    cout << "   MC1" << endl;
    cout << "   Tran Thanh Duy" << endl;
    cout << "========================================" << endl;

    testInsertSearch();
    testRemove();
    testEdgeCases();

    benchmark("../../src/product_inventory_100 000.csv", 100000);

    cout << "\n========================================" << endl;
    cout << "TAT CA TEST HASHTABLE DA PASS!" << endl;
    cout << "========================================" << endl;
    return 0;
}
