#include "SearchCore.h"
#include "Min_heap/Min_heap.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using namespace std;

string SearchCore::normalizeText(const string& text) {
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

string SearchCore::normalizeId(const string& rawId) {
    string id = normalizeText(rawId);
    if (!id.empty() && id.front() == '#') {
        id.erase(0, 1);
    }
    if (!id.empty() && id.front() == 'p') {
        id.front() = 'P';
    }
    return id;
}

bool SearchCore::isValidProductId(const string& id) {
    if (id.size() < 2 || id.front() != 'P') {
        return false;
    }
    for (size_t i = 1; i < id.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(id[i]))) {
            return false;
        }
    }
    try {
        return stoull(id.substr(1)) < numeric_limits<unsigned long long>::max();
    }
    catch (const exception&) {
        return false;
    }
}

bool SearchCore::isValidDate(const string& value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        return false;
    }
    for (size_t i = 0; i < value.size(); ++i) {
        if (i != 4 && i != 7 && !isdigit(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }

    const int year = stoi(value.substr(0, 4));
    const int month = stoi(value.substr(5, 2));
    const int day = stoi(value.substr(8, 2));
    if (year < 1900 || month < 1 || month > 12 || day < 1) {
        return false;
    }

    static const int days[] = {31, 28, 31, 30, 31, 30,
                               31, 31, 30, 31, 30, 31};
    int maxDay = days[month - 1];
    const bool leap = (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
    if (month == 2 && leap) {
        maxDay = 29;
    }
    return day <= maxDay;
}

bool SearchCore::isValidArrivedTime(const string& value) {
    if (value.size() == 10) {
        return isValidDate(value);
    }
    if (value.size() != 19 || value[10] != ' ' || value[13] != ':' ||
        value[16] != ':' || !isValidDate(value.substr(0, 10))) {
        return false;
    }
    for (size_t i : {11U, 12U, 14U, 15U, 17U, 18U}) {
        if (!isdigit(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }
    const int hour = stoi(value.substr(11, 2));
    const int minute = stoi(value.substr(14, 2));
    const int second = stoi(value.substr(17, 2));
    return hour <= 23 && minute <= 59 && second <= 59;
}

bool SearchCore::hasUnsafeCsvCharacters(const string& value) {
    return value.find_first_of(",\r\n") != string::npos;
}

bool SearchCore::validateProduct(Product& product, string& error) const {
    if (normalizeText(product.product_name).empty()) {
        error = "Ten san pham khong duoc rong.";
        return false;
    }
    if (hasUnsafeCsvCharacters(product.product_name)) {
        error = "Ten san pham khong duoc chua dau phay hoac xuong dong.";
        return false;
    }
    if (!isValidDate(product.made_date) ||
        !isValidArrivedTime(product.arrived_time) ||
        !isValidDate(product.best_by_date)) {
        error = "Ngay gio phai dung dinh dang ISO va la ngay hop le.";
        return false;
    }
    if (product.status != "AVAILABLE" && product.status != "RESERVED" &&
        product.status != "EXPIRED") {
        error = "Trang thai san pham khong hop le.";
        return false;
    }
    return true;
}

void SearchCore::updateIdCounter(const string& id) {
    const unsigned long long number = stoull(id.substr(1));
    if (number >= nextProductNumber) {
        nextProductNumber = number + 1;
    }
    idWidth = max(idWidth, static_cast<int>(id.size() - 1));
}

bool SearchCore::loadProducts(const vector<Product>& products, string& error) {
    if (productTable.getSize() != 0) {
        error = "SearchCore chi duoc nap du lieu mot lan.";
        return false;
    }

    unordered_set<string> ids;
    for (Product product : products) {
        if (!isValidProductId(product.id)) {
            error = "ID san pham khong hop le: " + product.id;
            return false;
        }
        if (!validateProduct(product, error)) {
            error = "Product " + product.id + ": " + error;
            return false;
        }
        if (!ids.insert(product.id).second) {
            error = "ID san pham bi trung: " + product.id;
            return false;
        }
    }

    for (const Product& product : products) {
        if (!productTable.insert(product)) {
            error = "Khong the nap Product vao Hash Table: " + product.id;
            return false;
        }
        productNameTrie.insert(normalizeText(product.product_name), product.id);
        updateIdCounter(product.id);
    }
    return true;
}

bool SearchCore::addProduct(Product& product, string& error) {
    if (nextProductNumber == numeric_limits<unsigned long long>::max()) {
        error = "Da het mien gia tri ID san pham.";
        return false;
    }

    Product created = product;
    created.status = "AVAILABLE";
    if (!validateProduct(created, error)) {
        return false;
    }

    ostringstream id;
    id << 'P' << setw(idWidth) << setfill('0') << nextProductNumber;
    created.id = id.str();
    if (!productTable.insert(created)) {
        error = "Khong the tao ID san pham duy nhat.";
        return false;
    }

    productNameTrie.insert(normalizeText(created.product_name), created.id);
    recentActions.Put(created, "ADD");
    updateIdCounter(created.id);
    product = created;
    return true;
}

bool SearchCore::deleteProduct(const string& rawId, string& error) {
    const string id = normalizeId(rawId);
    if (!isValidProductId(id)) {
        error = "ID san pham khong hop le.";
        return false;
    }

    Product* product = productTable.search(id);
    if (product == nullptr) {
        error = "Khong tim thay san pham.";
        return false;
    }

    const Product removed = *product;
    const string key = normalizeText(removed.product_name);
    if (!productNameTrie.remove(key, id)) {
        error = "Chi muc ten san pham khong dong bo.";
        return false;
    }
    if (!productTable.remove(id)) {
        productNameTrie.insert(key, id);
        error = "Khong the xoa san pham khoi Hash Table.";
        return false;
    }

    recentActions.Put(removed, "DELETE");
    return true;
}

bool SearchCore::reserveProduct(const string& rawId, Product& reserved,
                                string& error) {
    const string id = normalizeId(rawId);
    if (!isValidProductId(id)) {
        error = "ID san pham khong hop le.";
        return false;
    }

    Product* product = productTable.search(id);
    if (product == nullptr) {
        error = "Khong tim thay san pham.";
        return false;
    }
    if (product->status != "AVAILABLE") {
        error = "Chi san pham AVAILABLE moi duoc chuan bi.";
        return false;
    }

    product->status = "RESERVED";
    reserved = *product;
    recentActions.Put(reserved, "RESERVE");
    return true;
}

bool SearchCore::findById(const string& rawId, Product& product,
                          bool recordRecent) {
    const string id = normalizeId(rawId);
    if (!isValidProductId(id)) {
        return false;
    }
    Product* found = productTable.search(id);
    if (found == nullptr) {
        return false;
    }
    product = *found;
    if (recordRecent) {
        recentActions.Put(product, "READ");
    }
    return true;
}

vector<Product> SearchCore::previewPriority(const string& rawPrefix,
                                            size_t limit) {
    const string prefix = normalizeText(rawPrefix);
    if (prefix.empty() || limit == 0) {
        return {};
    }

    ProductMinHeap heap;
    for (const string& id : productNameTrie.searchByPrefix(prefix)) {
        Product* product = productTable.search(id);
        if (product != nullptr && product->status == "AVAILABLE") {
            heap.push(product);
        }
    }

    vector<Product> results;
    while (!heap.empty() && results.size() < limit) {
        results.push_back(*heap.pop());
    }
    return results;
}

vector<Product> SearchCore::search(const string& rawQuery, size_t limit) {
    if (limit == 0 || normalizeText(rawQuery).empty()) {
        return {};
    }

    Product exact;
    if (findById(rawQuery, exact, true)) {
        return {exact};
    }
    return previewPriority(rawQuery, limit);
}

vector<string> SearchCore::autocomplete(const string& rawPrefix,
                                        size_t limit) {
    const string prefix = normalizeText(rawPrefix);
    if (prefix.empty() || limit == 0) {
        return {};
    }

    vector<string> names;
    unordered_set<string> seen;
    for (const string& id : productNameTrie.searchByPrefix(prefix)) {
        const Product* product = productTable.search(id);
        if (product != nullptr && seen.insert(product->product_name).second) {
            names.push_back(product->product_name);
        }
    }
    sort(names.begin(), names.end());
    if (names.size() > limit) {
        names.resize(limit);
    }
    return names;
}

vector<CacheItem> SearchCore::getRecent() const {
    return recentActions.GetAll();
}

vector<Product> SearchCore::getAllProducts() const {
    vector<Product> products = productTable.values();
    sort(products.begin(), products.end(), [](const Product& left,
                                               const Product& right) {
        return left.id < right.id;
    });
    return products;
}

int SearchCore::size() const {
    return productTable.getSize();
}
