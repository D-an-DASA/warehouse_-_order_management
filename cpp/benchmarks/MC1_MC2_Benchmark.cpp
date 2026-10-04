#include "../DSAcore/Product.h"
#include "../DSAcore/hashtable/hashtable.h"
#include "../DSAcore/Min_heap/Min_heap.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace {

using Clock = chrono::steady_clock;

constexpr int WARMUP_RUNS = 3;
constexpr int MEASURED_RUNS = 15;
constexpr size_t MC1_QUERY_COUNT = 200;
constexpr size_t MC2_TOP_K = 100;

struct Stats {
    double averageUs;
    double medianUs;
};

// ------------------------------------------------------------
// Đọc CSV
// ------------------------------------------------------------

bool parseProductRow(const string& line, Product& product) {
    istringstream stream(line);
    vector<string> fields;
    string field;

    while (getline(stream, field, ',')) {
        fields.push_back(field);
    }

    if (fields.size() != 6) {
        return false;
    }

    product.id = fields[0];
    product.product_name = fields[1];
    product.made_date = fields[2];
    product.arrived_time = fields[3];
    product.best_by_date = fields[4];
    product.status = fields[5];

    return true;
}

vector<Product> loadProducts(
    const string& filename,
    size_t expectedCount
) {
    ifstream file(filename);

    if (!file) {
        throw runtime_error("Khong mo duoc dataset: " + filename);
    }

    string line;

    // Bỏ qua header.
    if (!getline(file, line)) {
        throw runtime_error("Dataset khong co header: " + filename);
    }

    vector<Product> products;
    products.reserve(expectedCount);

    size_t lineNumber = 1;

    while (getline(file, line)) {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        Product product;

        if (!parseProductRow(line, product)) {
            throw runtime_error(
                "CSV sai dinh dang tai dong " +
                to_string(lineNumber) +
                ": " + filename
            );
        }

        products.push_back(move(product));
    }

    if (products.size() != expectedCount) {
        throw runtime_error(
            "Dataset " + filename +
            " co " + to_string(products.size()) +
            " dong, mong doi " + to_string(expectedCount)
        );
    }

    return products;
}

// ------------------------------------------------------------
// Tính average và median
// ------------------------------------------------------------

double calculateAverage(const vector<double>& samples) {
    return accumulate(samples.begin(), samples.end(), 0.0)
        / static_cast<double>(samples.size());
}

double calculateMedian(vector<double> samples) {
    sort(samples.begin(), samples.end());

    const size_t middle = samples.size() / 2;

    if (samples.size() % 2 == 1) {
        return samples[middle];
    }

    return (samples[middle - 1] + samples[middle]) / 2.0;
}

template <typename Function>
Stats measureRepeated(
    Function function,
    size_t operationCount,
    size_t& checksum
) {
    if (operationCount == 0) {
        throw runtime_error("operationCount khong duoc bang 0");
    }

    // Warm-up: kết quả không được đưa vào thống kê.
    for (int i = 0; i < WARMUP_RUNS; ++i) {
        checksum += function();
    }

    vector<double> samples;
    samples.reserve(MEASURED_RUNS);

    for (int i = 0; i < MEASURED_RUNS; ++i) {
        const auto start = Clock::now();

        checksum += function();

        const auto finish = Clock::now();

        const double totalUs =
            chrono::duration<double, micro>(finish - start).count();

        // Chuẩn hóa về thời gian của một thao tác.
        samples.push_back(
            totalUs / static_cast<double>(operationCount)
        );
    }

    return {
        calculateAverage(samples),
        calculateMedian(samples)
    };
}

// ------------------------------------------------------------
// MC1 — Linear Scan và HashTable
// ------------------------------------------------------------

const Product* linearFind(
    const vector<Product>& products,
    const string& id
) {
    for (const Product& product : products) {
        if (product.id == id) {
            return &product;
        }
    }

    return nullptr;
}

vector<string> makeMC1Queries(const vector<Product>& products) {
    if (products.empty()) {
        throw runtime_error("Khong the tao query tu dataset rong");
    }

    vector<string> queries;
    queries.reserve(MC1_QUERY_COUNT);

    // Bắt buộc có đầu, giữa và cuối dataset.
    queries.push_back(products.front().id);
    queries.push_back(products[products.size() / 2].id);
    queries.push_back(products.back().id);

    mt19937 generator(20261004);
    uniform_int_distribution<size_t> distribution(
        0,
        products.size() - 1
    );

    // Tổng cộng 150 query có tồn tại.
    while (queries.size() < 150) {
        queries.push_back(
            products[distribution(generator)].id
        );
    }

    // Thêm 50 query không tồn tại.
    for (size_t i = 0; i < 50; ++i) {
        queries.push_back(
            "NOT_FOUND_" + to_string(i)
        );
    }

    shuffle(queries.begin(), queries.end(), generator);

    return queries;
}

size_t runLinearQueries(
    const vector<Product>& products,
    const vector<string>& queries
) {
    size_t result = 0;

    for (const string& id : queries) {
        const Product* product = linearFind(products, id);

        if (product != nullptr) {
            result += product->id.size();
        }
    }

    return result;
}

size_t runHashQueries(
    HashTable& table,
    const vector<string>& queries
) {
    size_t result = 0;

    for (const string& id : queries) {
        Product* product = table.search(id);

        if (product != nullptr) {
            result += product->id.size();
        }
    }

    return result;
}

void verifyMC1Correctness(
    const vector<Product>& products,
    HashTable& table,
    const vector<string>& queries
) {
    for (const string& id : queries) {
        const Product* linearResult = linearFind(products, id);
        Product* hashResult = table.search(id);

        const bool linearFound = linearResult != nullptr;
        const bool hashFound = hashResult != nullptr;

        if (linearFound != hashFound) {
            throw runtime_error(
                "MC1 sai ket qua voi ID: " + id
            );
        }

        if (
            linearResult != nullptr &&
            linearResult->id != hashResult->id
        ) {
            throw runtime_error(
                "MC1 tra ve Product khac nhau voi ID: " + id
            );
        }
    }
}

// ------------------------------------------------------------
// MC2 — Heap và cách naive
// ------------------------------------------------------------

vector<Product*> getAvailableProducts(
    vector<Product>& products
) {
    vector<Product*> candidates;
    candidates.reserve(products.size());

    for (Product& product : products) {
        if (product.status == "AVAILABLE") {
            candidates.push_back(&product);
        }
    }

    return candidates;
}

vector<string> selectTopKWithHeap(
    const vector<Product*>& candidates,
    size_t topK
) {
    ProductMinHeap heap;

    for (Product* product : candidates) {
        heap.push(product);
    }

    vector<string> result;
    result.reserve(topK);

    while (!heap.empty() && result.size() < topK) {
        Product* product = heap.pop();

        if (product == nullptr) {
            throw runtime_error("Heap tra ve con tro null");
        }

        result.push_back(product->id);
    }

    return result;
}

vector<string> selectTopKNaive(
    const vector<Product*>& candidates,
    size_t topK
) {
    vector<bool> used(candidates.size(), false);
    vector<string> result;
    result.reserve(topK);

    for (size_t selected = 0; selected < topK; ++selected) {
        size_t bestIndex = candidates.size();

        for (size_t i = 0; i < candidates.size(); ++i) {
            if (used[i]) {
                continue;
            }

            if (
                bestIndex == candidates.size() ||
                uutien(candidates[i], candidates[bestIndex])
            ) {
                bestIndex = i;
            }
        }

        if (bestIndex == candidates.size()) {
            break;
        }

        used[bestIndex] = true;
        result.push_back(candidates[bestIndex]->id);
    }

    return result;
}

size_t checksumIds(const vector<string>& ids) {
    size_t result = 0;

    for (const string& id : ids) {
        result += id.size();
    }

    return result;
}

void verifyMC2Correctness(
    const vector<Product*>& candidates,
    size_t topK
) {
    const vector<string> heapResult =
        selectTopKWithHeap(candidates, topK);

    const vector<string> naiveResult =
        selectTopKNaive(candidates, topK);

    if (heapResult != naiveResult) {
        throw runtime_error(
            "MC2: Heap va naive tra ve thu tu khac nhau"
        );
    }
}

// ------------------------------------------------------------
// Chạy benchmark cho một dataset
// ------------------------------------------------------------

void benchmarkDataset(
    const string& filename,
    size_t expectedCount
) {
    cout
        << "\n==================================================\n"
        << "DATASET: " << filename << '\n'
        << "==================================================\n";

    vector<Product> products =
        loadProducts(filename, expectedCount);

    cout << "Records: " << products.size() << '\n';

    size_t checksum = 0;

    // --------------------------------------------------------
    // Xây HashTable
    // --------------------------------------------------------

    HashTable table;

    const auto buildStart = Clock::now();

    for (const Product& product : products) {
        if (!table.insert(product)) {
            throw runtime_error(
                "Khong insert duoc Product vao HashTable: " +
                product.id
            );
        }
    }

    const auto buildFinish = Clock::now();

    const double hashBuildMs =
        chrono::duration<double, milli>(
            buildFinish - buildStart
        ).count();

    if (
        table.getSize() !=
        static_cast<int>(products.size())
    ) {
        throw runtime_error(
            "HashTable khong chua du toan bo Product"
        );
    }

    // --------------------------------------------------------
    // MC1
    // --------------------------------------------------------

    const vector<string> queries = makeMC1Queries(products);

    verifyMC1Correctness(products, table, queries);

    const Stats linearStats = measureRepeated(
        [&products, &queries]() {
            return runLinearQueries(products, queries);
        },
        queries.size(),
        checksum
    );

    const Stats hashStats = measureRepeated(
        [&table, &queries]() {
            return runHashQueries(table, queries);
        },
        queries.size(),
        checksum
    );

    cout
        << "\nMC1 - Exact ID lookup\n"
        << "Queries/run: " << queries.size() << '\n'
        << "HashTable build: "
        << fixed << setprecision(3)
        << hashBuildMs << " ms\n\n";

    cout
        << left
        << setw(15) << "Method"
        << right
        << setw(22) << "Average us/query"
        << setw(22) << "Median us/query"
        << '\n';

    cout
        << left
        << setw(15) << "Linear"
        << right
        << setw(22) << linearStats.averageUs
        << setw(22) << linearStats.medianUs
        << '\n';

    cout
        << left
        << setw(15) << "HashTable"
        << right
        << setw(22) << hashStats.averageUs
        << setw(22) << hashStats.medianUs
        << '\n';

    if (hashStats.medianUs > 0.0) {
        cout
            << "Median speedup: "
            << linearStats.medianUs / hashStats.medianUs
            << "x\n";
    }

    // --------------------------------------------------------
    // MC2
    // --------------------------------------------------------

    vector<Product*> candidates =
        getAvailableProducts(products);

    if (candidates.empty()) {
        throw runtime_error(
            "Dataset khong co Product AVAILABLE"
        );
    }

    const size_t topK = min(
        MC2_TOP_K,
        candidates.size()
    );

    verifyMC2Correctness(candidates, topK);

    const Stats naiveStats = measureRepeated(
        [&candidates, topK]() {
            return checksumIds(
                selectTopKNaive(candidates, topK)
            );
        },
        1,
        checksum
    );

    const Stats heapStats = measureRepeated(
        [&candidates, topK]() {
            return checksumIds(
                selectTopKWithHeap(candidates, topK)
            );
        },
        1,
        checksum
    );

    cout
        << "\nMC2 - Priority retrieval\n"
        << "AVAILABLE candidates: "
        << candidates.size() << '\n'
        << "Top K: " << topK << "\n\n";

    cout
        << left
        << setw(15) << "Method"
        << right
        << setw(22) << "Average us/run"
        << setw(22) << "Median us/run"
        << '\n';

    cout
        << left
        << setw(15) << "Naive"
        << right
        << setw(22) << naiveStats.averageUs
        << setw(22) << naiveStats.medianUs
        << '\n';

    cout
        << left
        << setw(15) << "MinHeap"
        << right
        << setw(22) << heapStats.averageUs
        << setw(22) << heapStats.medianUs
        << '\n';

    if (heapStats.medianUs > 0.0) {
        cout
            << "Median speedup: "
            << naiveStats.medianUs / heapStats.medianUs
            << "x\n";
    }

    // In checksum để compiler không loại bỏ phép tính.
    cout << "\nChecksum: " << checksum << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr
            << "Usage:\n"
            << "  mc1_mc2_benchmark "
            << "<dataset-10000> <dataset-100000>\n";

        return 1;
    }

    try {
        benchmarkDataset(argv[1], 10000);
        benchmarkDataset(argv[2], 100000);
    }
    catch (const exception& error) {
        cerr << "\nBENCHMARK FAILED: "
             << error.what() << '\n';

        return 2;
    }

    cout
        << "\n==================================================\n"
        << "BENCHMARK COMPLETED SUCCESSFULLY\n"
        << "Warm-up runs: " << WARMUP_RUNS << '\n'
        << "Measured runs: " << MEASURED_RUNS << '\n'
        << "==================================================\n";

    return 0;
}