#include "CsvProductRepository.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;

namespace {

vector<string> splitCsvLine(const string& line) {
    vector<string> fields;
    string field;
    istringstream input(line);
    while (getline(input, field, ',')) {
        fields.push_back(field);
    }
    if (!line.empty() && line.back() == ',') {
        fields.emplace_back();
    }
    return fields;
}

bool replaceFile(const fs::path& temporary, const fs::path& destination,
                 string& error) {
#ifdef _WIN32
    if (!MoveFileExW(temporary.wstring().c_str(), destination.wstring().c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Khong the thay the file CSV da luu.";
        return false;
    }
#else
    error_code code;
    fs::rename(temporary, destination, code);
    if (code) {
        error = "Khong the thay the file CSV da luu: " + code.message();
        return false;
    }
#endif
    return true;
}

} // namespace

bool CsvProductRepository::load(const string& filename,
                                vector<Product>& products,
                                string& error) const {
    ifstream file(filename, ios::binary);
    if (!file) {
        error = "Khong mo duoc file CSV: " + filename;
        return false;
    }

    string line;
    if (!getline(file, line)) {
        error = "File CSV rong: " + filename;
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    if (line != "id,product_name,made_date,arrived_time,best_by_date,status") {
        error = "Header CSV khong hop le.";
        return false;
    }

    vector<Product> loaded;
    size_t lineNumber = 1;
    while (getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const vector<string> fields = splitCsvLine(line);
        if (fields.size() != 6) {
            error = "CSV dong " + to_string(lineNumber) + ": can dung 6 cot.";
            return false;
        }
        loaded.push_back({fields[0], fields[1], fields[2], fields[3],
                          fields[4], fields[5]});
    }
    if (!file.eof()) {
        error = "Loi khi doc file CSV.";
        return false;
    }

    products = move(loaded);
    return true;
}

bool CsvProductRepository::save(const string& filename,
                                const vector<Product>& products,
                                string& error) const {
    const fs::path destination(filename);
    error_code code;
    if (destination.has_parent_path()) {
        fs::create_directories(destination.parent_path(), code);
        if (code) {
            error = "Khong tao duoc thu muc persistence: " + code.message();
            return false;
        }
    }

    const fs::path temporary = destination.string() + ".tmp";
    ofstream file(temporary, ios::binary | ios::trunc);
    if (!file) {
        error = "Khong tao duoc file CSV tam.";
        return false;
    }

    file << "id,product_name,made_date,arrived_time,best_by_date,status\n";
    for (const Product& product : products) {
        file << product.id << ',' << product.product_name << ','
             << product.made_date << ',' << product.arrived_time << ','
             << product.best_by_date << ',' << product.status << '\n';
    }
    file.flush();
    if (!file.good()) {
        file.close();
        fs::remove(temporary, code);
        error = "Khong ghi duoc day du du lieu CSV.";
        return false;
    }
    file.close();

    if (!replaceFile(temporary, destination, error)) {
        fs::remove(temporary, code);
        return false;
    }
    return true;
}
