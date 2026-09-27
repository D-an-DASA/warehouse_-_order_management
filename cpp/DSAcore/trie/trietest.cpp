#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include "trie/trie.h"
#include "product.h" 

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
// TEST 1: INSERT + SEARCH
// ============================================================
void testInsertAndSearch() {
    cout << "\n=== Test 1: Insert and Search ===" << endl;
    Trie trie;
    trie.insert("iPhone 15", "P001");
    trie.insert("iPhone 15 Pro", "P002");
    trie.insert("iPhone 16", "P003");
    trie.insert("Samsung S24", "P004");

    auto r = trie.searchByPrefix("iph");
    cout << "  'iph': " << r.size() << " ket qua" << endl;
    assert(r.size() == 3);

    r = trie.searchByPrefix("iPhone 15");
    cout << "  'iPhone 15': " << r.size() << " ket qua" << endl;
    assert(r.size() == 2);

    r = trie.searchByPrefix("Xiaomi");
    assert(r.empty());
    cout << "  'Xiaomi': 0 ket qua" << endl;

    cout << "  => PASSED!" << endl;
}

// ============================================================
// TEST 2: REMOVE
// ============================================================
void testRemove() {
    cout << "\n=== Test 2: Remove ===" << endl;
    Trie trie;
    trie.insert("iPhone 15", "P001");
    trie.insert("iPhone 15 Pro", "P002");

    assert(trie.remove("iPhone 15", "P001") == true);
    auto r = trie.searchByPrefix("iPhone 15");
    assert(r.size() == 1 && r[0] == "P002");
    cout << "  Xoa P001 thanh cong" << endl;

    assert(trie.remove("Samsung", "P999") == false);
    cout << "  Xoa khong ton tai -> OK" << endl;

    cout << "  => PASSED!" << endl;
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
    trie.insert("iPhone 15", "P001");
    trie.insert("Samsung", "P002");
    auto r = trie.searchByPrefix("");
    assert(r.size() == 2);
    cout << "  Tim '': " << r.size() << " ket qua" << endl;

    r = trie.searchByPrefix("iPhone 15 Pro Max");
    assert(r.empty());
    cout << "  Prefix dai hon ten -> OK" << endl;

    Trie trie2;
    trie2.insert("iPhone 15", "P001");
    trie2.insert("iPhone 15", "P002");
    r = trie2.searchByPrefix("iPhone 15");
    assert(r.size() == 2);
    cout << "  Nhieu san pham cung ten: " << r.size() << " ket qua" << endl;

    cout << "  => PASSED!" << endl;
}

// ============================================================
// BENCHMARK
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

    cout << "  So san pham: " << products.size() << " (mong doi " << expected << ")" << endl;
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

    // Search Trie
    vector<string> prefixes = {"P", "USB", "Shampoo", "Green", "Lap"};
    int iter = 1000;

    t0 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++)
        for (auto& p : prefixes)
            trie.searchByPrefix(p);
    t1 = high_resolution_clock::now();
    auto trieUs = duration_cast<microseconds>(t1 - t0).count();
    double avgTrie = (double)trieUs / (iter * prefixes.size());
    cout << "  [Trie] Search (" << iter * prefixes.size() << " lan): " 
         << trieUs << " us" << endl;
    cout << "  [Trie] Trung binh: " << avgTrie << " us/lan" << endl;

    // Search Linear
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
    double avgLinear = (double)linearUs / (iter * prefixes.size());
    cout << "  [Linear] Search (" << iter * prefixes.size() << " lan): " 
         << linearUs << " us" << endl;
    cout << "  [Linear] Trung binh: " << avgLinear << " us/lan" << endl;

    if (trieUs > 0) {
        double speedup = (double)linearUs / trieUs;
        cout << "\n  ==> Trie nhanh hon linear scan " << speedup << " lan" << endl;
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    cout << "========================================" << endl;
    cout << "   TEST & BENCHMARK TRIE - YC 4.2" << endl;
    cout << "   Nguoi thuc hien: Duong Minh Phuc" << endl;
    cout << "========================================" << endl;

    testInsertAndSearch();
    testRemove();
    testEdgeCases();

    benchmark("../data/products_10k.csv", 10000);
    // benchmark("../data/products_100k.csv", 100000);  // bỏ comment khi có file

    cout << "\n========================================" << endl;
    cout << "TAT CA TEST TRIE DA PASS!" << endl;
    cout << "========================================" << endl;
    return 0;
}
