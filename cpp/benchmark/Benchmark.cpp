#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "../DSAcore/Min_Heap/Min_heap.h"
#include "../DSAcore/SearchCore.h"
#include "../Persistence/CsvProductRepository.h"

using namespace std;
using Clock = chrono::steady_clock;

namespace {

string normalize(const string& value) {
    string result;
    bool pendingSpace = false;
    for (unsigned char character : value) {
        if (isspace(character)) {
            pendingSpace = !result.empty();
        } else {
            if (pendingSpace) result += ' ';
            result += static_cast<char>(tolower(character));
            pendingSpace = false;
        }
    }
    return result;
}

bool startsWith(const string& value, const string& prefix) {
    return value.size() >= prefix.size() &&
           equal(prefix.begin(), prefix.end(), value.begin());
}

uint64_t mixChecksum(uint64_t checksum, const string& id) {
    for (unsigned char character : id) {
        checksum = (checksum * 1099511628211ULL) ^ character;
    }
    return checksum;
}

vector<Product> naivePriority(const vector<Product>& products,
                              const string& rawPrefix, size_t limit) {
    const string prefix = normalize(rawPrefix);
    vector<Product> matches;
    for (const Product& product : products) {
        if (product.status == "AVAILABLE" &&
            startsWith(normalize(product.product_name), prefix)) {
            matches.push_back(product);
        }
    }
    sort(matches.begin(), matches.end(), [](const Product& left,
                                             const Product& right) {
        return uutien(&left, &right);
    });
    if (matches.size() > limit) matches.resize(limit);
    return matches;
}

template <typename Function>
pair<long long, uint64_t> measure(Function function) {
    const auto started = Clock::now();
    const uint64_t checksum = function();
    const auto elapsed = chrono::duration_cast<chrono::nanoseconds>(
        Clock::now() - started).count();
    return {elapsed, checksum};
}

void printRow(const string& dataset, size_t records, const string& phase,
              const string& operation, const string& implementation,
              int repeat, size_t queries, long long elapsed,
              uint64_t checksum) {
    cout << dataset << ',' << records << ',' << phase << ',' << operation << ','
         << implementation << ',' << repeat << ',' << queries << ',' << elapsed
         << ',' << checksum << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 6) {
        cerr << "Usage: Benchmark <csv> <label> <records> <repeats> <seed>\n";
        return 1;
    }
    const string csvPath = argv[1];
    const string label = argv[2];
    const size_t expectedRecords = stoull(argv[3]);
    const int repeats = stoi(argv[4]);
    const unsigned int seed = stoul(argv[5]);

    CsvProductRepository repository;
    vector<Product> products;
    string error;
    const auto loadStarted = Clock::now();
    if (!repository.load(csvPath, products, error)) {
        cerr << error << '\n';
        return 1;
    }
    const auto loadElapsed = chrono::duration_cast<chrono::nanoseconds>(
        Clock::now() - loadStarted).count();
    if (products.size() != expectedRecords) {
        cerr << "Sai so ban ghi: " << products.size() << '\n';
        return 1;
    }

    SearchCore core;
    const auto buildStarted = Clock::now();
    if (!core.loadProducts(products, error)) {
        cerr << error << '\n';
        return 1;
    }
    const auto buildElapsed = chrono::duration_cast<chrono::nanoseconds>(
        Clock::now() - buildStarted).count();

    mt19937 generator(seed);
    const size_t mc1QueryCount = min<size_t>(2000, products.size() / 5);
    vector<string> idQueries;
    idQueries.reserve(mc1QueryCount);
    uniform_int_distribution<size_t> pick(0, products.size() - 1);
    for (size_t index = 0; index < mc1QueryCount; ++index) {
        idQueries.push_back(index % 5 == 0
                                ? "P999999999" + to_string(index)
                                : products[pick(generator)].id);
    }

    vector<string> prefixQueries;
    unordered_set<string> names;
    for (const Product& product : products) {
        if (names.insert(product.product_name).second) {
            prefixQueries.push_back(product.product_name);
            if (prefixQueries.size() == 20) break;
        }
    }

    auto optimizedMc1 = [&]() {
        uint64_t checksum = 1469598103934665603ULL;
        Product found;
        for (const string& id : idQueries) {
            if (core.findById(id, found, false)) checksum = mixChecksum(checksum, found.id);
        }
        return checksum;
    };
    auto naiveMc1 = [&]() {
        uint64_t checksum = 1469598103934665603ULL;
        for (const string& id : idQueries) {
            const auto found = find_if(products.begin(), products.end(),
                                       [&](const Product& product) {
                                           return product.id == id;
                                       });
            if (found != products.end()) checksum = mixChecksum(checksum, found->id);
        }
        return checksum;
    };
    auto optimizedMc2 = [&]() {
        uint64_t checksum = 1469598103934665603ULL;
        for (const string& prefix : prefixQueries) {
            for (const Product& product : core.previewPriority(prefix, 20)) {
                checksum = mixChecksum(checksum, product.id);
            }
        }
        return checksum;
    };
    auto naiveMc2 = [&]() {
        uint64_t checksum = 1469598103934665603ULL;
        for (const string& prefix : prefixQueries) {
            for (const Product& product : naivePriority(products, prefix, 20)) {
                checksum = mixChecksum(checksum, product.id);
            }
        }
        return checksum;
    };

    if (optimizedMc1() != naiveMc1() || optimizedMc2() != naiveMc2()) {
        cerr << "Checksum khong tuong duong giua hai implementation.\n";
        return 1;
    }

    cout << "dataset,records,phase,operation,implementation,repeat,queries,elapsed_ns,checksum\n";
    printRow(label, products.size(), "setup", "csv_load", "repository", 0,
             products.size(), loadElapsed, products.size());
    printRow(label, products.size(), "setup", "index_build", "optimized", 0,
             products.size(), buildElapsed, products.size());

    for (int repeat = 1; repeat <= repeats; ++repeat) {
        const auto optimized1 = measure(optimizedMc1);
        const auto naive1 = measure(naiveMc1);
        const auto optimized2 = measure(optimizedMc2);
        const auto naive2 = measure(naiveMc2);
        if (optimized1.second != naive1.second || optimized2.second != naive2.second) {
            cerr << "Checksum khong khop o repeat " << repeat << ".\n";
            return 1;
        }
        printRow(label, products.size(), "query", "mc1_exact_id", "hash_table",
                 repeat, idQueries.size(), optimized1.first, optimized1.second);
        printRow(label, products.size(), "query", "mc1_exact_id", "linear_scan",
                 repeat, idQueries.size(), naive1.first, naive1.second);
        printRow(label, products.size(), "query", "mc2_priority", "trie_min_heap",
                 repeat, prefixQueries.size(), optimized2.first, optimized2.second);
        printRow(label, products.size(), "query", "mc2_priority", "filter_sort",
                 repeat, prefixQueries.size(), naive2.first, naive2.second);
    }
    return 0;
}
