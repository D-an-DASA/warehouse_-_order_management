#ifndef MIN_HEAP_H
#define MIN_HEAP_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Product.h"
using namespace std;
vector<string> splistCSV(const string& line);
bool uutien(const Product* a, const Product* b);
// MIN HEAP
class ProductMinHeap {
private:
    vector<Product*> heap;
    int root(int index);
    int leftChild(int index);
    int rightChild(int index);

    void swapNode(int a, int b);

    void heapifyUp(int index);
    void heapifyDown(int index);

public:
    bool empty() const;
    int size() const;
    void push(Product* product);
    Product* top() const;
    Product* pop();
};
// INVENTORY MANAGER
class InventoryManager {
private:
    vector<Product> products;
    unordered_map<string, ProductMinHeap> heaps;
    unordered_map<string, Product*> productById;
public:
    bool loadCSV(const string& filename);
    int size() const;
    int countAvailable() const;
    void cleanTop(const string& productName);
    Product* peekBest(const string& productName);
    Product* selectBest(const string& productName);
    bool releaseProduct(const string& id);
    bool expireProduct(const string& id);
};
// In thông tin Product
void printProduct(const Product* product);
#endif
