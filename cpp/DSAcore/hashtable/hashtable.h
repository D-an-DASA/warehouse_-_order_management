// ==========================================================
// hastable.h - Hash Table tra cứu sản phẩm theo ID (O(1))
// Nguoi thuc hien: Tran Thanh Duy - MC1
// ==========================================================
#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <vector>
#include <list>
#include <string>
#include "../Product.h"

using namespace std;

class HashTable {
private:
    vector<list<Product>> buckets;
    int capacity;
    int size;

    int hashFunc(const string& id, int cap);
    void resize();

public:
    HashTable();
    ~HashTable();

    void insert(const Product& p);
    Product* search(const string& id);
    bool remove(const string& id);

    int getSize() const { return size; }
    int getCapacity() const { return capacity; }
};

#endif
