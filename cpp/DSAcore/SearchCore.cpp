#include "SearchCore.h"
#include "Min_heap/Min_heap.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace {

bool csvRecordComplete(const string& record) {
    bool quoted = false;
    for (size_t i = 0; i < record.size(); ++i) {
        if (record[i] != '"') {
            continue;
        }
        if (quoted && i + 1 < record.size() && record[i + 1] == '"') {
            ++i;
        }
        else {
            quoted = !quoted;
        }
    }
    return !quoted;
}

bool parseCsvRow(const string& line, vector<string>& fields) {
    fields.clear();
    string field;
    bool quoted = false;
    bool closedQuote = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                }
                else {
                    quoted = false;
                    closedQuote = true;
                }
            }
            else {
                field += ch;
            }
        }
        else if (ch == ',' && !quoted) {
            fields.push_back(field);
            field.clear();
            closedQuote = false;
        }
        else if (ch == '"') {
            if (!field.empty() || closedQuote) {
                return false;
            }
            quoted = true;
        }
        else if (closedQuote) {
            return false;
        }
        else {
            field += ch;
        }
    }

    if (quoted) {
        return false;
    }
    fields.push_back(field);
    return true;
}

string escapeCsvField(const string& field) {
    if (field.find_first_of(",\"\r\n") == string::npos) {
        return field;
    }

    string escaped = "\"";
    for (char ch : field) {
        if (ch == '"') {
            escaped += '"';
        }
        escaped += ch;
    }
    escaped += '"';
    return escaped;
}

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
    size_t recordStartLine = 0;
    bool readingRecord = false;
    string record;
    while (getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!readingRecord && normalizeText(line).empty()) {
            continue;
        }
        if (readingRecord) {
            record += '\n';
        }
        else {
            recordStartLine = lineNumber;
            readingRecord = true;
        }
        record += line;
        if (!csvRecordComplete(record)) {
            continue;
        }

        vector<string> fields;
        if (!parseCsvRow(record, fields) || fields.size() != 6) {
            cerr << "CSV dong " << recordStartLine
                 << ": can dung 6 cot CSV hop le\n";
            return false;
        }
        record.clear();
        readingRecord = false;

        Product product;
        product.id = fields[0];
        product.product_name = fields[1];
        product.made_date = fields[2];
        product.arrived_time = fields[3];
        product.best_by_date = fields[4];
        product.status = fields[5];

        if (!isValidProductId(product.id)) {
            cerr << "CSV dong " << recordStartLine << ": ID khong hop le\n";
            return false;
        }
        if (!productTable.insert(product)) {
            cerr << "CSV dong " << recordStartLine
                 << ": ID san pham bi trung: " << product.id << '\n';
            return false;
        }

        productNameTrie.insert(normalizeText(product.product_name), product.id);
        updateIdCounter(product.id);
    }

    if (readingRecord) {
        cerr << "CSV dong " << recordStartLine
             << ": record CSV chua dong quote\n";
        return false;
    }
    return file.eof();
}

// Ghi snapshot đầy đủ của HashTable ra CSV.
bool SearchCore::saveCSV(const string& filename) const {
    const filesystem::path path(filename);
    const filesystem::path parent = path.parent_path();
    error_code filesystemError;
    if (!parent.empty()) {
        filesystem::create_directories(parent, filesystemError);
        if (filesystemError) {
            cerr << "Khong tao duoc thu muc CSV: " << parent.string()
                 << ": " << filesystemError.message() << '\n';
            return false;
        }
    }

    ofstream file(path, ios::binary | ios::trunc);
    if (!file) {
        cerr << "Khong mo duoc file CSV de ghi: " << filename << '\n';
        return false;
    }

    file << "id,product_name,made_date,arrived_time,best_by_date,status\n";
    vector<Product> products = productTable.getAll();
    sort(products.begin(), products.end(),
         [](const Product& left, const Product& right) {
             return left.id < right.id;
         });

    for (const Product& product : products) {
        file << escapeCsvField(product.id) << ','
             << escapeCsvField(product.product_name) << ','
             << escapeCsvField(product.made_date) << ','
             << escapeCsvField(product.arrived_time) << ','
             << escapeCsvField(product.best_by_date) << ','
             << escapeCsvField(product.status) << '\n';
    }

    file.close();
    if (!file) {
        cerr << "Ghi file CSV that bai: " << filename << '\n';
        return false;
    }
    return true;
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

// Goi y ten khong trung lap, sap thu tu truoc khi lay limit.
vector<string> SearchCore::autocomplete(
    const string& prefix,
    size_t limit
) {
    const string key = normalizeText(prefix);
    if (key.empty() || limit == 0) {
        return {};
    }

    vector<string> names;
    unordered_set<string> seen;
    for (const string& id : productNameTrie.searchByPrefix(key)) {
        Product* product = productTable.search(id);
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

// Lay thao tac gan nhat cua tung ID theo thu tu LRU.
vector<CacheItem> SearchCore::getRecent() const {
    return recentActions.GetAll();
}

// Lay so Product truc tiep tu nguon du lieu chinh.
int SearchCore::size() const {
    return productTable.getSize();
}
