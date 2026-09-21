#include <iostream>
#include <cassert>
#include <vector>
#include "LRU_Cache.h"

using namespace std;

int main()
{
    cout << "===== LRU Cache Test =====\n\n";

    // --------------------------------------------------
    // TEST 1: Cache ban đầu rỗng
    // --------------------------------------------------
    {
        LRU_Cache cache;

        assert(cache.Size() == 0);

        cout << "[PASS] Initial cache is empty\n";
    }


    // --------------------------------------------------
    // TEST 2: Put product
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product p1;
        p1.id = "P001";

        cache.Put(p1, "CREATE");

        assert(cache.Size() == 1);
        assert(cache.IsContain(p1));

        cout << "[PASS] Put and IsContain\n";
    }


    // --------------------------------------------------
    // TEST 3: Thứ tự khi thêm nhiều product
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product p1;
        Product p2;
        Product p3;

        p1.id = "P001";
        p2.id = "P002";
        p3.id = "P003";

        cache.Put(p1, "CREATE");
        cache.Put(p2, "CREATE");
        cache.Put(p3, "CREATE");

        vector<CacheItem> items = cache.GetAll();

        assert(items.size() == 3);

        // Product mới nhất nằm ở đầu
        assert(items[0].product.id == "P003");
        assert(items[1].product.id == "P002");
        assert(items[2].product.id == "P001");

        cout << "[PASS] Insert order (MRU -> LRU)\n";
    }


    // --------------------------------------------------
    // TEST 4: Truy cập lại product -> Move_Front
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product p1;
        Product p2;
        Product p3;

        p1.id = "P001";
        p2.id = "P002";
        p3.id = "P003";

        cache.Put(p1, "CREATE");
        cache.Put(p2, "CREATE");
        cache.Put(p3, "CREATE");

        // P1 đang ở cuối.
        // Truy cập lại P1 -> P1 phải lên đầu.
        cache.Put(p1, "READ");

        vector<CacheItem> items = cache.GetAll();

        assert(items.size() == 3);

        assert(items[0].product.id == "P001");
        assert(items[1].product.id == "P003");
        assert(items[2].product.id == "P002");

        // Operation cũng phải được update
        assert(items[0].operation == "READ");

        cout << "[PASS] Move accessed item to front\n";
    }


    // --------------------------------------------------
    // TEST 5: Eviction khi vượt MAX_SIZE
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product products[11];

        for (int i = 0; i < 11; i++)
        {
            products[i].id = "P" + to_string(i + 1);
            cache.Put(products[i], "CREATE");
        }

        // MAX_SIZE = 10
        assert(cache.Size() == 10);

        // P1 là product cũ nhất -> phải bị remove
        assert(!cache.IsContain(products[0]));

        // P2 vẫn còn
        assert(cache.IsContain(products[1]));

        // P11 là product mới nhất
        assert(cache.IsContain(products[10]));

        cout << "[PASS] Cache eviction at MAX_SIZE\n";
    }


    // --------------------------------------------------
    // TEST 6: Kiểm tra thứ tự sau eviction
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product products[11];

        for (int i = 0; i < 11; i++)
        {
            products[i].id = "P" + to_string(i + 1);
            cache.Put(products[i], "CREATE");
        }

        vector<CacheItem> items = cache.GetAll();

        assert(items.size() == 10);

        // MRU -> LRU
        assert(items[0].product.id == "P11");
        assert(items[1].product.id == "P10");
        assert(items[2].product.id == "P9");

        // P2 phải là LRU cuối cùng
        assert(items[9].product.id == "P2");

        cout << "[PASS] Correct order after eviction\n";
    }


    // --------------------------------------------------
    // TEST 7: Truy cập phần tử ở giữa
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product p1;
        Product p2;
        Product p3;
        Product p4;

        p1.id = "P001";
        p2.id = "P002";
        p3.id = "P003";
        p4.id = "P004";

        cache.Put(p1, "CREATE");
        cache.Put(p2, "CREATE");
        cache.Put(p3, "CREATE");
        cache.Put(p4, "CREATE");

        // Thứ tự hiện tại:
        // P4 -> P3 -> P2 -> P1

        cache.Put(p2, "UPDATE");

        // Sau khi truy cập P2:
        // P2 -> P4 -> P3 -> P1

        vector<CacheItem> items = cache.GetAll();

        assert(items[0].product.id == "P002");
        assert(items[1].product.id == "P004");
        assert(items[2].product.id == "P003");
        assert(items[3].product.id == "P001");

        assert(items[0].operation == "UPDATE");

        cout << "[PASS] Move middle item to front\n";
    }


    cout << "\n===== All tests passed! =====\n";

    return 0;
}