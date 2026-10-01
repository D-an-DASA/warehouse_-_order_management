#include <cassert>
#include <iostream>
#include <vector>

#include "Min_heap.h"

using namespace std;

Product product(const string& id, const string& bestBy,
                const string& arrived) {
    return {id, "Sua tuoi", "2026-01-01", arrived, bestBy, "AVAILABLE"};
}

int main() {
    vector<Product> products = {
        product("P00003", "2026-10-03", "2026-01-01 08:00:00"),
        product("P00002", "2026-10-02", "2026-01-01 09:00:00"),
        product("P00001", "2026-10-02", "2026-01-01 09:00:00"),
        product("P00004", "2026-10-02", "2026-01-01 07:00:00")
    };

    ProductMinHeap heap;
    for (Product& item : products) {
        heap.push(&item);
    }

    assert(heap.size() == 4);
    assert(heap.pop()->id == "P00004");
    assert(heap.pop()->id == "P00001");
    assert(heap.pop()->id == "P00002");
    assert(heap.pop()->id == "P00003");
    assert(heap.pop() == nullptr);

    cout << "Min_heaptest: PASS\n";
    return 0;
}
