#include <cassert>
#include <iostream>
#include <string>

#include "hashtable.h"

using namespace std;

Product product(int number) {
    return {"P" + to_string(number), "San pham", "2026-01-01",
            "2026-01-02 08:00:00", "2026-12-31", "AVAILABLE"};
}

int main() {
    HashTable table;
    const int originalCapacity = table.getCapacity();
    for (int i = 1; i <= 200; ++i) {
        assert(table.insert(product(i)));
    }

    assert(table.getSize() == 200);
    assert(table.getCapacity() > originalCapacity);
    assert(table.search("P150") != nullptr);
    assert(table.search("P150")->id == "P150");
    assert(!table.insert(product(150)));
    assert(table.remove("P150"));
    assert(table.search("P150") == nullptr);
    assert(!table.remove("P150"));
    assert(table.values().size() == 199);

    cout << "HashTableTest: PASS\n";
    return 0;
}
