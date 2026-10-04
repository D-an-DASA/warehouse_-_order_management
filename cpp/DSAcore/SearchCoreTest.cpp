#include "SearchCore.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

#ifdef NDEBUG
#error SearchCoreTest requires assertions; do not compile with NDEBUG.
#endif

using namespace std;
namespace fs = std::filesystem;

namespace {

int passed = 0;
const string header =
    "id,product_name,made_date,arrived_time,best_by_date,status\n";
const string sampleRows =
    "P00001,Power Bank,2025-01-01,2026-01-01 08:00:00,2028-01-01,AVAILABLE\n"
    "P00002,Power Bank,2025-01-01,2026-02-01 08:00:00,2027-01-01,RESERVED\n"
    "P00003,Power Bank,2025-01-01,2026-01-01 08:00:00,2027-01-01,EXPIRED\n"
    "P00004,Power Bank,2025-01-01,2026-01-01 08:00:00,2027-01-01,AVAILABLE\n"
    "P00005,Power Cable,2025-01-01,2026-01-01 08:00:00,2029-01-01,AVAILABLE\n";

void pass(const string& name) {
    cout << "PASS " << ++passed << ": " << name << endl;
}

// Moi fixture la mot file rieng trong thu muc tam cua lan chay nay.
string writeCSV(const fs::path& directory, const string& contents) {
    static int fileNumber = 0;
    fs::path path = directory / (to_string(++fileNumber) + ".csv");
    ofstream file(path, ios::binary);
    file << contents;
    file.close();
    assert(file.good());
    return path.string();
}

vector<string> idsOf(const vector<Product>& products) {
    vector<string> ids;
    for (const Product& product : products) {
        ids.push_back(product.id);
    }
    return ids;
}

Product makeProduct(const string& name, const string& status = "") {
    return {"", name, "2026-01-01", "2026-01-02 08:00:00",
            "2027-01-01", status};
}

void testMainFlows(const fs::path& directory) {
    SearchCore core;
    assert(core.loadCSV(writeCSV(directory, header + sampleRows)));
    assert(core.getRecent().empty());
    pass("loadCSV thanh cong, khong ghi recent");

    assert(core.size() == 5);
    pass("size dung sau loadCSV");

    vector<Product> found = core.search("P00001");
    assert(found.size() == 1);
    assert(found[0].id == "P00001");
    assert(found[0].product_name == "Power Bank");
    assert(found[0].made_date == "2025-01-01");
    assert(found[0].arrived_time == "2026-01-01 08:00:00");
    assert(found[0].best_by_date == "2028-01-01");
    assert(found[0].status == "AVAILABLE");
    pass("tim ID va giu du 6 truong Product");

    assert(core.search("Power B").size() == 2);
    assert(core.search("ower").empty());
    pass("tim theo prefix, khong tim substring");

    assert(idsOf(core.search("  POWER \t  bank  ")) ==
           idsOf(core.search("power bank")));
    assert(core.search("  POWER \t  bank  ").size() == 2);
    pass("normalize chu hoa, trim va gom khoang trang");

    assert(core.search("#P00001").at(0).id == "P00001");
    assert(core.search("p00001").at(0).id == "P00001");
    assert(core.search("  #p00001 \t").at(0).id == "P00001");
    pass("chap nhan ID thuong va ID co dau #");

    assert((core.autocomplete("  POWER  ") ==
            vector<string>{"Power Bank", "Power Cable"}));
    pass("autocomplete tra ten hien thi theo thu tu on dinh");

    assert((core.autocomplete("power b") == vector<string>{"Power Bank"}));
    pass("autocomplete khong lap ten cua nhieu Product");

    Product added = makeProduct("Power Adapter");
    assert(core.addProduct(added));
    assert(added.id == "P00006");
    assert(added.status == "AVAILABLE");
    pass("addProduct sinh ID tiep theo va status mac dinh");

    assert(core.size() == 6);
    assert(core.search(added.id).at(0).product_name == "Power Adapter");
    assert((idsOf(core.search("power a")) == vector<string>{added.id}));
    assert((core.autocomplete("power a") == vector<string>{"Power Adapter"}));
    pass("addProduct dong bo HashTable va Trie");

    assert(core.deleteProduct(added.id));
    assert(core.size() == 5);
    assert(core.search(added.id).empty());
    pass("deleteProduct xoa khoi HashTable");

    assert(core.search("power a").empty());
    assert(core.autocomplete("power a").empty());
    pass("deleteProduct loai ket qua prefix va autocomplete");

    assert(core.deleteProduct("P00003"));
    assert((idsOf(core.search("power bank")) ==
            vector<string>{"P00004", "P00001"}));
    assert((core.autocomplete("power b") == vector<string>{"Power Bank"}));
    pass("xoa mot ID van giu cac Product cung ten");

    SearchCore ranked;
    assert(ranked.loadCSV(writeCSV(directory, header + sampleRows)));
    assert((idsOf(ranked.search("power bank")) ==
            vector<string>{ "P00004", "P00001"}));
    pass("Min Heap uu tien best_by_date, arrived_time, roi ID");

    assert((idsOf(ranked.search("power bank", 1)) == vector<string>{"P00004"}));
    assert((idsOf(ranked.search("power bank", 2)) ==
            vector<string>{"P00004", "P00001"}));
    assert(ranked.search("power", 100).size() == 3);
    pass("search limit duoc ap dung sau xep hang");

    assert((ranked.autocomplete("power", 1) == vector<string>{"Power Bank"}));
    assert(ranked.autocomplete("power", 100).size() == 2);
    pass("autocomplete limit duoc ap dung sau sap ten");

    SearchCore recent;
    Product item = makeProduct("Recent Item");
    assert(recent.addProduct(item));
    assert(recent.getRecent().size() == 1);
    assert(recent.getRecent()[0].operation == "ADD");
    assert(!recent.getRecent()[0].time.empty());
    assert(recent.deleteProduct(item.id));
    const vector<CacheItem> actions = recent.getRecent();
    assert(actions.size() == 1);
    assert(actions[0].operation == "DELETE");
    assert(actions[0].product.id == item.id);
    assert(actions[0].product.product_name == item.product_name);
    assert(!actions[0].time.empty());
    assert(recent.size() == 0);
    pass("recent ADD/DELETE giu ban copy va thao tac moi nhat theo ID");

    assert(ranked.search("").empty());
    assert(ranked.search(" \t\n ").empty());
    assert(ranked.search("power", 0).empty());
    assert(ranked.search("P00001", 0).empty());
    assert(ranked.autocomplete("").empty());
    assert(ranked.autocomplete(" \t ").empty());
    assert(ranked.autocomplete("power", 0).empty());
    assert(ranked.getRecent().empty());
    pass("query rong va limit 0 khong tao ket qua hoac recent");

    assert(ranked.search("P77777").empty());
    assert(ranked.search("#").empty());
    assert(ranked.autocomplete("absent").empty());
    assert(!ranked.deleteProduct("P77777"));
    assert(!recent.deleteProduct(item.id));
    assert(ranked.size() == 5);
    pass("tim va xoa ID khong ton tai an toan");

    assert(recent.addProduct(item));
    const vector<CacheItem> before = recent.getRecent();
    assert(recent.search("recent").size() == 1);
    assert(recent.autocomplete("recent").size() == 1);
    const vector<CacheItem> after = recent.getRecent();
    assert(before.size() == after.size());
    for (size_t i = 0; i < before.size(); ++i) {
        assert(before[i].product.id == after[i].product.id);
        assert(before[i].operation == after[i].operation);
        assert(before[i].time == after[i].time);
    }
    pass("prefix search va autocomplete khong thay doi recent");

    assert(recent.search(item.id).size() == 1);
    assert(recent.getRecent()[0].operation == "READ");
    assert(recent.getRecent()[0].product.id == item.id);
    pass("exact ID search ghi READ");

    SearchCore statuses;
    for (const string status : {"AVAILABLE", "RESERVED", "EXPIRED", "invalid", ""}) {
        Product product = makeProduct("Status Item", status);
        assert(statuses.addProduct(product));
        const string expected = (status == "invalid" || status.empty())
                                    ? "AVAILABLE" : status;
        assert(product.status == expected);
        assert(statuses.search(product.id).at(0).status == expected);
    }
    assert(statuses.search("status").size() == 3);
    pass("MC1 tra ID moi status, MC2 chi tra AVAILABLE");

    SearchCore display;
    Product spaced = makeProduct("  Power   Bank ");
    assert(display.addProduct(spaced));
    assert(display.search("POWER BANK").at(0).product_name == "  Power   Bank ");
    assert(display.autocomplete("power bank").at(0) == "  Power   Bank ");
    assert(display.deleteProduct(spaced.id));
    assert(display.autocomplete("power bank").empty());
    pass("normalize index khi them/xoa, giu nguyen ten hien thi");
}

void testIdAndResize(const fs::path& directory) {
    SearchCore rollover;
    assert(rollover.loadCSV(writeCSV(directory, header +
        "P99999,Boundary,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\n")));
    Product product = makeProduct("Boundary");
    assert(rollover.addProduct(product));
    assert(product.id == "P100000");
    assert(rollover.addProduct(product));
    assert(product.id == "P100001");
    pass("tang ID qua P99999");

    SearchCore sixDigits;
    assert(sixDigits.loadCSV(writeCSV(directory, header +
        "P000100,Six Digits,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\n"
        "P000002,Six Digits,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\n")));
    assert(sixDigits.addProduct(product));
    assert(product.id == "P000101");
    pass("giu do rong 6 chu so va lay ID lon nhat du CSV khong sap thu tu");

    SearchCore resized;
    vector<string> expectedIds;
    for (int i = 0; i < 200; ++i) {
        Product added = makeProduct("Resize Item");
        assert(resized.addProduct(added));
        expectedIds.push_back(added.id);
    }
    assert(resized.size() == 200);
    const vector<Product> snapshot = resized.search("resize", 500);
    assert(idsOf(snapshot) == expectedIds);
    for (const string& id : expectedIds) {
        assert(resized.search(id).at(0).id == id);
    }
    assert(resized.deleteProduct(expectedIds.front()));
    assert(resized.deleteProduct(expectedIds.back()));
    expectedIds.erase(expectedIds.begin());
    expectedIds.pop_back();
    assert(idsOf(resized.search("resize", 500)) == expectedIds);
    assert(snapshot.size() == 200);
    assert(snapshot.front().id == "P00001");
    pass("HashTable resize, search lai, xoa va ban copy ket qua an toan");

    SearchCore lru;
    vector<string> addedIds;
    for (int i = 0; i < 12; ++i) {
        Product added = makeProduct("LRU Item");
        assert(lru.addProduct(added));
        addedIds.push_back(added.id);
    }
    vector<CacheItem> actions = lru.getRecent();
    assert(actions.size() == 10);
    for (size_t i = 0; i < actions.size(); ++i) {
        assert(actions[i].product.id == addedIds[11 - i]);
    }
    assert(lru.search(addedIds[2]).size() == 1);
    assert(lru.getRecent().front().product.id == addedIds[2]);
    assert(lru.deleteProduct(addedIds[0]));
    assert(lru.getRecent().size() == 10);
    assert(lru.getRecent().front().product.id == addedIds[0]);
    assert(lru.getRecent().front().operation == "DELETE");
    pass("LRU toi da 10 ID, cap nhat thu tu va ghi DELETE cho ID da bi day ra");
}

void testCsvErrors(const fs::path& directory) {
    SearchCore missing;
    assert(!missing.loadCSV((directory / "missing.csv").string()));
    assert(!missing.loadCSV(writeCSV(directory, "")));
    assert(missing.size() == 0);
    assert(missing.loadCSV(writeCSV(directory, header)));
    pass("CSV khong ton tai, file rong va chi co header");

    SearchCore partial;
    assert(!partial.loadCSV(writeCSV(directory, header + sampleRows +
        "P00006,Missing Columns\n")));
    assert(partial.size() == 5);
    assert(partial.search("power").size() == 3);
    assert(partial.search("P00006").empty());
    assert(partial.getRecent().empty());
    Product next = makeProduct("After Error");
    assert(partial.addProduct(next));
    assert(next.id == "P00006");
    SearchCore extra;
    assert(!extra.loadCSV(writeCSV(directory, header +
        "P00001,Extra,a,b,c,AVAILABLE,unexpected\n")));
    assert(extra.size() == 0);
    pass("CSV sai so cot: giu dong hop le, khong insert dong loi");

    for (const string id : {"", "P", "X00001", "p00001", "P12x", "P-1",
                             "P18446744073709551615", "P18446744073709551616"}) {
        SearchCore invalid;
        assert(!invalid.loadCSV(writeCSV(directory, header + id +
            ",Bad ID,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\n")));
        assert(invalid.size() == 0);
        assert(invalid.search("bad").empty());
        assert(invalid.getRecent().empty());
        Product first = makeProduct("Valid");
        assert(invalid.addProduct(first));
        assert(first.id == "P00001");
    }
    pass("ID sai dang hoac vuot gioi han khong gay exception hay mat dong bo");

    SearchCore duplicate;
    assert(!duplicate.loadCSV(writeCSV(directory, header + sampleRows +
        "P00001,Wrong Name,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\n")));
    assert(duplicate.size() == 5);
    assert(duplicate.search("P00001").at(0).product_name == "Power Bank");
    assert(duplicate.search("wrong").empty());
    assert(duplicate.autocomplete("wrong").empty());
    pass("CSV ID trung khong ghi de Product hoac them index sai");

    SearchCore exhausted;
    const string lastUsable = "P" +
        to_string(numeric_limits<unsigned long long>::max() - 1);
    assert(exhausted.loadCSV(writeCSV(directory, header + lastUsable +
        ",Last,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\n")));
    Product unchanged = makeProduct("Cannot Add", "EXPIRED");
    unchanged.id = "original";
    assert(!exhausted.addProduct(unchanged));
    assert(unchanged.id == "original");
    assert(unchanged.status == "EXPIRED");
    assert(exhausted.size() == 1);
    assert(exhausted.getRecent().empty());
    pass("bo dem het mien gia tri tra false, khong tran va khong sua dau vao");

    SearchCore crlf;
    assert(crlf.loadCSV(writeCSV(directory,
        "id,product_name,made_date,arrived_time,best_by_date,status\r\n"
        "\r\n \t\r\n"
        "P00001,  Power   Bank ,2026-01-01,2026-01-02,2027-01-01,AVAILABLE\r\n")));
    assert(crlf.size() == 1);
    const vector<Product> found = crlf.search("power bank");
    assert(found.size() == 1);
    assert(found[0].status == "AVAILABLE");
    assert(found[0].product_name == "  Power   Bank ");
    assert(crlf.deleteProduct("P00001"));
    assert(crlf.autocomplete("power").empty());
    pass("CSV CRLF, dong trang va ten nhieu khoang trang");
}

void testSaveCSV(const fs::path& directory) {
    SearchCore original;
    Product quoted = makeProduct("Product, \"Quoted\"\nName", "EXPIRED");
    quoted.made_date = "2026-02-03";
    quoted.arrived_time = "2026-02-04 05:06:07";
    quoted.best_by_date = "2026-02-05";
    assert(original.addProduct(quoted));

    Product reserved = makeProduct("Reserved Item", "RESERVED");
    assert(original.addProduct(reserved));

    const fs::path filename =
        directory / "Persistent" / "Persistent.csv";
    assert(original.saveCSV(filename.string()));
    assert(fs::exists(filename));

    SearchCore restored;
    assert(restored.loadCSV(filename.string()));
    assert(restored.size() == 2);

    const Product savedQuoted = restored.search(quoted.id).at(0);
    assert(savedQuoted.id == quoted.id);
    assert(savedQuoted.product_name == quoted.product_name);
    assert(savedQuoted.made_date == quoted.made_date);
    assert(savedQuoted.arrived_time == quoted.arrived_time);
    assert(savedQuoted.best_by_date == quoted.best_by_date);
    assert(savedQuoted.status == quoted.status);
    assert(restored.search(reserved.id).at(0).status == "RESERVED");
    pass("saveCSV tao thu muc, luu day du truong va round-trip CSV escaping");
}

void testPersistInventoryDataset(const fs::path& directory) {
    const fs::path source =
        fs::path("cpp") / "product_inventory_10 000.csv";
    const fs::path destination =
        directory / "Persistent" / "Persistent.csv";

    SearchCore inventory;
    assert(inventory.loadCSV(source.string()));
    assert(inventory.size() == 10000);
    assert(inventory.saveCSV(destination.string()));

    SearchCore restored;
    assert(restored.loadCSV(destination.string()));
    assert(restored.size() == inventory.size());
    assert(restored.search("P00001").at(0).id == "P00001");
    assert(restored.search("P10000").at(0).id == "P10000");
    pass("luu dataset 10,000 san pham vao Persistent.csv va nap lai day du");
}

void testDataset(const string& filename, int count, const string& lastId,
                 const string& nextId) {
    SearchCore core;
    assert(core.loadCSV(filename));
    assert(core.size() == count);
    assert(core.getRecent().empty());
    assert(core.search(lastId).at(0).id == lastId);
    assert(!core.search("power bank").empty());
    assert((core.autocomplete("power bank") == vector<string>{"Power Bank"}));
    Product added = makeProduct("Dataset New Item");
    assert(core.addProduct(added));
    assert(added.id == nextId);
    assert(core.deleteProduct(added.id));
    assert(core.size() == count);
    pass("dataset that " + to_string(count) + " Product va ID tiep theo");
}

} // namespace

int main() {
    const auto runId = chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory = fs::temp_directory_path() /
        ("SearchCoreTest-" + to_string(runId));
    if (!fs::create_directory(directory)) {
        cerr << "Khong tao duoc thu muc test rieng\n";
        return 1;
    }

    try {
        testMainFlows(directory);
        testIdAndResize(directory);
        testCsvErrors(directory);
        testSaveCSV(directory);
        testPersistInventoryDataset(directory);
        testDataset("cpp/product_inventory_10 000.csv", 10000, "P10000", "P10001");
        testDataset("cpp/product_inventory_100 000.csv", 100000, "P100000", "P100001");
    }
    catch (const exception& error) {
        cerr << "FAIL: " << error.what() << '\n';
        fs::remove_all(directory);
        return 1;
    }

    fs::remove_all(directory);
    cout << "TOTAL: " << passed << '/' << passed << " PASS\n";
    return 0;
}
