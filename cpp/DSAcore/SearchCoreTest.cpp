#include "SearchCore.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#ifdef NDEBUG
#error SearchCoreTest requires assertions; do not compile with NDEBUG.
#endif

using namespace std;

namespace {

int passed = 0;

void pass(const string& name) {
    cout << "PASS " << ++passed << ": " << name << '\n';
}

Product product(const string& id, const string& name,
                const string& bestBy, const string& arrived,
                const string& status) {
    return {id, name, "2025-01-01", arrived, bestBy, status};
}

vector<string> idsOf(const vector<Product>& products) {
    vector<string> ids;
    for (const Product& item : products) {
        ids.push_back(item.id);
    }
    return ids;
}

vector<Product> sampleProducts() {
    return {
        product("P00001", "Power Bank", "2028-01-01",
                "2026-01-01 08:00:00", "AVAILABLE"),
        product("P00002", "Power Bank", "2027-01-01",
                "2026-02-01 08:00:00", "RESERVED"),
        product("P00003", "Power Bank", "2026-01-01",
                "2026-01-01 08:00:00", "EXPIRED"),
        product("P00004", "Power Bank", "2027-01-01",
                "2026-01-01 08:00:00", "AVAILABLE"),
        product("P00005", "Power Cable", "2029-01-01",
                "2026-01-01 08:00:00", "AVAILABLE")
    };
}

void testLoadAndMc1() {
    SearchCore core;
    string error;
    assert(core.loadProducts(sampleProducts(), error));
    assert(core.size() == 5);

    Product found;
    assert(core.findById("  #p00001 ", found));
    assert(found.id == "P00001");
    assert(found.product_name == "Power Bank");
    assert(core.getRecent().size() == 1);
    assert(core.getRecent()[0].operation == "READ");

    assert(core.findById("P00003", found));
    assert(found.status == "EXPIRED");
    assert(!core.findById("P99999", found));
    assert(!core.findById("bad-id", found));
    pass("MC1 tra cuu exact ID, chuan hoa va not-found");
}

void testMc2Preview() {
    SearchCore core;
    string error;
    assert(core.loadProducts(sampleProducts(), error));

    const vector<Product> preview = core.previewPriority(" power bank ", 20);
    assert((idsOf(preview) == vector<string>{"P00004", "P00001"}));
    for (const Product& item : preview) {
        assert(item.status == "AVAILABLE");
    }
    assert(core.getRecent().empty());

    assert((idsOf(core.previewPriority("POWER", 2)) ==
            vector<string>{"P00004", "P00001"}));
    assert(core.previewPriority("power", 1).size() == 1);
    assert(core.previewPriority("missing", 20).empty());
    assert(core.previewPriority("", 20).empty());
    assert(core.previewPriority("power", 0).empty());
    pass("MC2 chi preview AVAILABLE va dung thu tu ba khoa");
}

void testPriorityTieBreaks() {
    SearchCore core;
    string error;
    const vector<Product> products = {
        product("P00003", "Milk", "2027-01-01", "2026-02-01", "AVAILABLE"),
        product("P00002", "Milk", "2027-01-01", "2026-01-01", "AVAILABLE"),
        product("P00001", "Milk", "2027-01-01", "2026-01-01", "AVAILABLE")
    };
    assert(core.loadProducts(products, error));
    assert((idsOf(core.previewPriority("milk")) ==
            vector<string>{"P00001", "P00002", "P00003"}));
    pass("MC2 tie-break arrived_time roi id");
}

void testTrieAndSynchronization() {
    SearchCore core;
    string error;
    assert(core.loadProducts(sampleProducts(), error));
    assert((core.autocomplete(" power ") ==
            vector<string>{"Power Bank", "Power Cable"}));
    assert((core.autocomplete("power", 1) == vector<string>{"Power Bank"}));

    Product added{"", "Power Adapter", "2026-01-01", "2026-01-02",
                  "2027-01-01", ""};
    assert(core.addProduct(added, error));
    assert(added.id == "P00006");
    assert(added.status == "AVAILABLE");
    Product found;
    assert(core.findById(added.id, found, false));
    assert((core.autocomplete("power a") == vector<string>{"Power Adapter"}));
    assert(idsOf(core.previewPriority("power a")) == vector<string>{"P00006"});

    assert(core.deleteProduct("#p00006", error));
    assert(!core.findById("P00006", added, false));
    assert(core.autocomplete("power a").empty());
    assert(core.previewPriority("power a").empty());
    assert(core.getRecent()[0].operation == "DELETE");
    pass("Hash Table va Trie dong bo sau add/delete");
}

void testReserveRemovesFromPriorityButKeepsHashEntry() {
    SearchCore core;
    string error;
    assert(core.loadProducts({
        product("P00001", "Sua tuoi", "2026-10-05",
                "2026-01-01 08:00:00", "AVAILABLE"),
        product("P00002", "Sua tuoi", "2026-10-06",
                "2026-01-01 08:00:00", "AVAILABLE"),
        product("P00003", "Sua tuoi", "2026-10-04",
                "2026-01-01 08:00:00", "EXPIRED")
    }, error));

    Product reserved;
    assert(core.reserveProduct("p00001", reserved, error));
    assert(reserved.id == "P00001");
    assert(reserved.status == "RESERVED");

    const vector<Product> priority = core.previewPriority("sua tuoi");
    assert(priority.size() == 1);
    assert(priority[0].id == "P00002");

    Product exact;
    assert(core.findById("P00001", exact, false));
    assert(exact.status == "RESERVED");

    assert(!core.reserveProduct("P00001", reserved, error));
    assert(error == "Chi san pham AVAILABLE moi duoc chuan bi.");
    assert(!core.reserveProduct("P00003", reserved, error));

    const vector<CacheItem> recent = core.getRecent();
    assert(!recent.empty());
    assert(recent.front().operation == "RESERVE");
    pass("Reserve cap nhat Hash Table va loai khoi previewPriority");
}

void testValidationAndAtomicLoad() {
    string error;
    SearchCore invalidLoad;
    vector<Product> invalid = sampleProducts();
    invalid.push_back(product("bad", "Bad", "2027-01-01",
                              "2026-01-01", "AVAILABLE"));
    assert(!invalidLoad.loadProducts(invalid, error));
    assert(invalidLoad.size() == 0);

    SearchCore duplicate;
    vector<Product> duplicated = sampleProducts();
    duplicated.push_back(duplicated.front());
    assert(!duplicate.loadProducts(duplicated, error));
    assert(duplicate.size() == 0);

    SearchCore core;
    assert(core.loadProducts({}, error));
    Product badName{"", "   ", "2026-01-01", "2026-01-02",
                    "2027-01-01", "AVAILABLE"};
    assert(!core.addProduct(badName, error));
    Product badDate{"", "Valid", "2026-02-30", "2026-01-02",
                    "2027-01-01", "AVAILABLE"};
    assert(!core.addProduct(badDate, error));
    Product csvInjection{"", "Bad,Name", "2026-01-01", "2026-01-02",
                         "2027-01-01", "AVAILABLE"};
    assert(!core.addProduct(csvInjection, error));
    assert(core.size() == 0);
    pass("validation va load loi khong de lai du lieu mot phan");
}

void testResizeAndGeneratedIds() {
    SearchCore core;
    string error;
    vector<string> ids;
    for (int i = 0; i < 200; ++i) {
        Product item{"", "Resize Item", "2026-01-01", "2026-01-02",
                     "2027-01-01", ""};
        assert(core.addProduct(item, error));
        ids.push_back(item.id);
    }
    assert(core.size() == 200);
    assert(core.previewPriority("resize", 500).size() == 200);
    for (const string& id : ids) {
        Product found;
        assert(core.findById(id, found, false));
    }
    pass("Hash Table resize van giu MC1, Trie va Min Heap preview");
}

void testRecentCapacityFifty() {
    SearchCore core;
    string error;
    vector<string> ids;
    for (int i = 0; i < 52; ++i) {
        Product item{"", "Recent Item", "2026-01-01", "2026-01-02",
                     "2027-01-01", ""};
        assert(core.addProduct(item, error));
        ids.push_back(item.id);
    }
    const vector<CacheItem> recent = core.getRecent();
    assert(recent.size() == 50);
    assert(recent.front().product.id == ids.back());
    assert(recent.back().product.id == ids[2]);
    Product found;
    assert(core.findById(ids[10], found));
    assert(core.getRecent().front().product.id == ids[10]);
    pass("Recent Workspace gioi han 50 va dua READ len dau");
}

} // namespace

int main() {
    testLoadAndMc1();
    testMc2Preview();
    testPriorityTieBreaks();
    testTrieAndSynchronization();
    testReserveRemovesFromPriorityButKeepsHashEntry();
    testValidationAndAtomicLoad();
    testResizeAndGeneratedIds();
    testRecentCapacityFifty();
    cout << "TOTAL: " << passed << '/' << passed << " PASS\n";
    return 0;
}
