#include <cassert>
#include <iostream>
#include <string>

#include "LRU_Cache.h"

using namespace std;

Product makeProduct(int number) {
    Product product;
    product.id = "P" + to_string(number);
    product.product_name = "San pham " + to_string(number);
    product.status = "AVAILABLE";
    return product;
}

int main() {
    LRU_Cache cache;
    for (int i = 1; i <= 51; ++i) {
        cache.Put(makeProduct(i), "READ");
    }

    assert(cache.Size() == 50);
    assert(!cache.IsContain("P1"));
    assert(cache.IsContain("P51"));

    cache.Put(makeProduct(2), "RESERVE");
    const vector<CacheItem> items = cache.GetAll();
    assert(items.size() == 50);
    assert(items.front().product.id == "P2");
    assert(items.front().operation == "RESERVE");

    assert(cache.Remove("P2"));
    assert(!cache.Remove("P2"));
    assert(cache.Size() == 49);

    cout << "LRU_CacheTest: PASS\n";
    return 0;
}
