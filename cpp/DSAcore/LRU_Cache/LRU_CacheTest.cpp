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
        assert(cache.IsContain(p1.id));

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
        assert(!cache.IsContain(products[0].id));

        // P2 vẫn còn
        assert(cache.IsContain(products[1].id));

        // P11 là product mới nhất
        assert(cache.IsContain(products[10].id));

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

    // --------------------------------------------------
    // TEST 8: Cap nhat snapshot Product cu
    // --------------------------------------------------
    {
        LRU_Cache cache;

        Product p1;
        p1.id = "P001";
        p1.product_name = "Old Product Name";
        p1.status = "AVAILABLE";
        p1.quantity = 1;

        Product p2;
        p2.id = "P002";

        cache.Put(p1, "CREATE");
        cache.Put(p2, "CREATE");

        // P001 dang o cuoi danh sach.
        p1.product_name = "Updated Product Name";
        p1.status = "RESERVED";
        p1.quantity = 7;

        cache.Put(p1, "UPDATE");

        vector<CacheItem> items = cache.GetAll();

        assert(items.size() == 2);
        assert(items[0].product.id == "P001");
        assert(items[0].product.product_name == "Updated Product Name");
        assert(items[0].product.status == "RESERVED");
        assert(items[0].product.quantity == 7);
        assert(items[0].operation == "UPDATE");

        // P001 hien dang la head. Lan Put nay kiem tra Product
        // van duoc cap nhat truoc nhanh return som trong Move_Front.
        p1.quantity = 9;
        cache.Put(p1, "READ");

        items = cache.GetAll();

        assert(items[0].product.quantity == 9);
        assert(items[0].operation == "READ");

        cout << "[PASS] Refresh cached Product snapshot\n";
    }

    // --------------------------------------------------
    // TEST 9: Remove head, middle, tail, single, missing
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

        // Thu tu: P004 -> P003 -> P002 -> P001

        assert(cache.Remove("P004"));
        assert(!cache.IsContain(p4.id));

        vector<CacheItem> items = cache.GetAll();
        assert(items.size() == 3);
        assert(items[0].product.id == "P003");
        assert(items[2].product.id == "P001");

        assert(cache.Remove("P002"));
        assert(!cache.IsContain(p2.id));

        items = cache.GetAll();
        assert(items.size() == 2);
        assert(items[0].product.id == "P003");
        assert(items[1].product.id == "P001");

        assert(cache.Remove("P001"));
        assert(!cache.IsContain(p1.id));

        items = cache.GetAll();
        assert(items.size() == 1);
        assert(items[0].product.id == "P003");

        assert(!cache.Remove("P999"));

        assert(cache.Remove("P003"));
        assert(cache.Size() == 0);
        assert(cache.GetAll().empty());

        // Cache phai van dung duoc sau khi bi xoa rong.
        cache.Put(p2, "READ");
        assert(cache.Size() == 1);
        assert(cache.IsContain(p2.id));

        cout << "[PASS] Remove head, middle, tail and single item\n";
    }

    // --------------------------------------------------
    // TEST 10: Remove tao cho trong cache
    // --------------------------------------------------
    {
        LRU_Cache cache;
        Product products[11];

        for (int i = 0; i < 10; i++)
        {
            products[i].id = "P" + to_string(i + 1);
            cache.Put(products[i], "CREATE");
        }

        assert(cache.Size() == 10);
        assert(cache.Remove("P5"));
        assert(cache.Size() == 9);

        products[10].id = "P11";
        cache.Put(products[10], "CREATE");

        assert(cache.Size() == 10);
        assert(cache.IsContain(products[0].id));
        assert(!cache.IsContain(products[4].id));
        assert(cache.IsContain(products[10].id));

        cout << "[PASS] Remove frees one cache slot\n";
    }

    cout << "\n===== All tests passed! =====\n";

    return 0;
}
