#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "CsvProductRepository.h"

using namespace std;
namespace fs = std::filesystem;

void testRoundTrip(const fs::path& directory) {
    const vector<Product> expected = {
        {"P00001", "Sua tuoi", "2026-01-01", "2026-01-02 08:00:00",
         "2026-02-01", "AVAILABLE"},
        {"P00002", "Banh mi", "2026-01-01", "2026-01-02 09:00:00",
         "2026-01-10", "RESERVED"}
    };

    CsvProductRepository repository;
    string error;
    const fs::path file = directory / "inventory.csv";
    assert(repository.save(file.string(), expected, error));

    vector<Product> actual;
    assert(repository.load(file.string(), actual, error));
    assert(actual.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(actual[i].id == expected[i].id);
        assert(actual[i].product_name == expected[i].product_name);
        assert(actual[i].made_date == expected[i].made_date);
        assert(actual[i].arrived_time == expected[i].arrived_time);
        assert(actual[i].best_by_date == expected[i].best_by_date);
        assert(actual[i].status == expected[i].status);
    }
}

void testRejectsInvalidCsv(const fs::path& directory) {
    const fs::path file = directory / "invalid.csv";
    ofstream output(file);
    output << "wrong,header\nP00001,Sua tuoi\n";
    output.close();

    CsvProductRepository repository;
    vector<Product> products;
    string error;
    assert(!repository.load(file.string(), products, error));
    assert(error == "Header CSV khong hop le.");
}

void testOfficialDatasets() {
    CsvProductRepository repository;
    vector<Product> products;
    string error;

    assert(repository.load("cpp/product_inventory_10 000.csv", products, error));
    assert(products.size() == 10000);
    assert(products.front().id == "P00001");

    assert(repository.load("cpp/product_inventory_100 000.csv", products, error));
    assert(products.size() == 100000);
    assert(products.front().id == "P000001");
}

int main() {
    const auto suffix = chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory = fs::temp_directory_path() /
                               ("warehouse_repository_test_" + to_string(suffix));
    fs::create_directories(directory);

    testRoundTrip(directory);
    testRejectsInvalidCsv(directory);
    testOfficialDatasets();

    fs::remove_all(directory);
    cout << "CsvProductRepositoryTest: PASS\n";
    return 0;
}
