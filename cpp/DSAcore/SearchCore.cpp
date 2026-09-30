#include "SearchCore.h"
#include "Min_heap/Min_heap.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace {

// Chuan hoa khoa tim kiem, khong thay doi ten hien thi trong Product.
string normalizeText(const string& text) {
    string result;
    bool pendingSpace = false;

    for (unsigned char ch : text) {
        if (isspace(ch)) {
            pendingSpace = !result.empty();
            continue;
        }
        if (pendingSpace) {
            result += ' ';
            pendingSpace = false;
        }
        result += static_cast<char>(tolower(ch));
    }
    return result;
}

// Kiem tra ID va chua lai mot gia tri de bo dem khong bi tran.
bool isValidProductId(const string& id) {
    if (id.size() < 2 || id.front() != 'P') {
        return false;
    }
    for (size_t i = 1; i < id.size(); ++i) {
        if (id[i] < '0' || id[i] > '9') {
            return false;
        }
    }
    try {
        return stoull(id.substr(1)) <
               numeric_limits<unsigned long long>::max();
    }
    catch (const invalid_argument&) {
        return false;
    }
    catch (const out_of_range&) {
        return false;
    }
}

} // namespace

// Cap nhat so thu tu va do rong tu ID da duoc kiem tra.
void SearchCore::updateIdCounter(const string& id) {
    const unsigned long long number = stoull(id.substr(1));
    if (number >= nextProductNumber) {
        nextProductNumber = number + 1;
    }

    const int width = static_cast<int>(id.size() - 1);
    if (width > idWidth) {
        idWidth = width;
    }
}

// Doc CSV vao HashTable va Trie; giu cac dong hop le neu gap dong loi.
bool SearchCore::loadCSV(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Khong mo duoc file CSV: " << filename << '\n';
        return false;
    }

    string line;
    if (!getline(file, line)) { // Bo qua dong tieu de.
        cerr << "Khong doc duoc header CSV: " << filename << '\n';
        return false;
    }

    size_t lineNumber = 1;
    while (getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (normalizeText(line).empty()) {
            continue;
        }
        if (count(line.begin(), line.end(), ',') != 5) {
            cerr << "CSV dong " << lineNumber << ": can dung 6 cot\n";
            return false;
        }

        istringstream row(line);
        Product product;
        getline(row, product.id, ',');
        getline(row, product.product_name, ',');
        getline(row, product.made_date, ',');
        getline(row, product.arrived_time, ',');
        getline(row, product.best_by_date, ',');
        getline(row, product.status, ',');

        if (!isValidProductId(product.id)) {
            cerr << "CSV dong " << lineNumber << ": ID khong hop le\n";
            return false;
        }
        if (!productTable.insert(product)) {
            cerr << "CSV dong " << lineNumber
                 << ": ID san pham bi trung: " << product.id << '\n';
            return false;
        }

        productNameTrie.insert(normalizeText(product.product_name), product.id);
        updateIdCounter(product.id);
    }

    return file.eof();
}

// Them mot Product va dong bo index ten, recent.
bool SearchCore::addProduct(Product& product) {
    if (nextProductNumber == numeric_limits<unsigned long long>::max()) {
        return false;
    }
    ostringstream id;
    id << 'P' << setw(idWidth) << setfill('0')
       << nextProductNumber;

    Product created = product;
    created.id = id.str();
    if (created.status != "AVAILABLE" &&
        created.status != "RESERVED" &&
        created.status != "EXPIRED") {
        created.status = "AVAILABLE";
    }
    if (!productTable.insert(created)) {
        return false;
    }

    productNameTrie.insert(normalizeText(created.product_name), created.id);
    recentActions.Put(created, "ADD");
    updateIdCounter(created.id);
    product = created;
    return true;
}

// Xoa Product, dong bo Trie va luu ban copy cho thao tac DELETE.
bool SearchCore::deleteProduct(const string& id) {
    Product* product = productTable.search(id);
    if (product == nullptr) {
        return false;
    }

    const Product removed = *product;
    const string key = normalizeText(removed.product_name);
    if (!productNameTrie.remove(key, id)) {
        return false;
    }

    if (!productTable.remove(id)) {
        productNameTrie.insert(key, id);
        return false;
    }
    recentActions.Put(removed, "DELETE");
    return true;
}

// Tim theo ID hoac prefix, xep ket qua bang Min Heap.
vector<Product> SearchCore::search(
    const string& query,
    size_t limit
) {
    const string key = normalizeText(query);
    if (key.empty() || limit == 0) {
        return {};
    }

    string id = key;
    if (id.front() == '#') {
        id.erase(0, 1);
    }
    if (!id.empty() && id.front() == 'p') {
        id.front() = 'P';
    }
    Product* exactMatch = productTable.search(id);
    if (exactMatch != nullptr) {
        recentActions.Put(*exactMatch, "READ");
        return {*exactMatch};
    }

    // Heap chi song trong ham; khong them/xoa khi dang dung pointer HashTable.
    // Resize cua HashTable co the lam pointer cu mat hieu luc.
    ProductMinHeap heap;
    for (const string& productId : productNameTrie.searchByPrefix(key)) {
        Product* product = productTable.search(productId);
        if (product != nullptr) {
            heap.push(product);
        }
    }

    vector<Product> results;
    while (!heap.empty() && results.size() < limit) {
        results.push_back(*heap.pop());
    }
    return results;
}
