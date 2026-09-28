#include <iostream>
#include <sstream>
#include <fstream>
#include "Min_heap.h"
using namespace std;
// TÁCH CSV
vector<string> splistCSV(const string& line) {
    vector<string> result;
    string value;
    stringstream ss(line);
    while (getline(ss, value, ',')) {
        result.push_back(value);
    }
    return result;
}
// SO SÁNH ƯU TIÊN
bool uutien(const Product* a, const Product* b) {
    if (a->best_by_date != b->best_by_date) {
        return a->best_by_date < b->best_by_date;
    }
    if (a->arrived_time != b->arrived_time) {
        return a->arrived_time < b->arrived_time;
    }
    return a->id < b->id;
}
// MIN HEAP
int ProductMinHeap::root(int index) {
    return (index - 1) / 2;
}
int ProductMinHeap::leftChild(int index) {
    return index * 2 + 1;
}

int ProductMinHeap::rightChild(int index) {
    return index * 2 + 2;
}

void ProductMinHeap::swapNode(int a, int b) {

    Product* temp = heap[a];

    heap[a] = heap[b];

    heap[b] = temp;
}

void ProductMinHeap::heapifyUp(int index) {
    while (index > 0) {
        int p = root(index);
        if (uutien(heap[index], heap[p])) {
            swapNode(index, p);
            index = p;
        }
        else {
            break;
        }
    }
}
void ProductMinHeap::heapifyDown(int index) {
    int n = heap.size();
    while (true) {
        int left = leftChild(index);
        int right = rightChild(index);
        int smallest = index;
        if (left < n &&uutien(heap[left], heap[smallest])) {
            smallest = left;
        }

        if (right < n &&uutien(heap[right], heap[smallest])) {
            smallest = right;
        }

        if (smallest == index) {
            break;
        }
        swapNode(index, smallest);
        index = smallest;
    }
}
bool ProductMinHeap::empty() const {
    return heap.empty();
}
int ProductMinHeap::size() const {
    return heap.size();
}
void ProductMinHeap::push(Product* product) {

    heap.push_back(product);

    heapifyUp(heap.size() - 1);
}
Product* ProductMinHeap::top() const {
    if (heap.empty()) {
        return nullptr;
    }
    return heap[0];
}
Product* ProductMinHeap::pop() {
    if (heap.empty()) {
        return nullptr;
    }
    Product* result = heap[0];
    heap[0] = heap.back();
    heap.pop_back();
    if (!heap.empty()) {
        heapifyDown(0);
    }
    return result;
}
// LOAD CSV
bool InventoryManager::loadCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Khong the mo file: "<< filename << '\n';
        return false;
    }
    string line;
    getline(file, line);
    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        vector<string> fields = splistCSV(line);
        if (fields.size() != 6) {
            continue;
        }
        Product product;
        product.id = fields[0];
        product.product_name = fields[1];
        product.made_date = fields[2];
        product.arrived_time = fields[3];
        product.best_by_date = fields[4];
        product.status = fields[5];
        products.push_back(product);
    }
    file.close();
    for (Product& product : products) {
        productById[product.id] = &product;
        if (product.status == "AVAILABLE") {
            heaps[product.product_name].push(&product);
        }
    }
    return true;
}
// SỐ LƯỢNG PRODUCT
int InventoryManager::size() const {

    return products.size();
}
// ĐẾM AVAILABLE
int InventoryManager::countAvailable() const {
    int count = 0;
    for (const Product& product : products) {
        if (product.status == "AVAILABLE") {
            count++;
        }
    }
    return count;
}
// CLEAN TOP
void InventoryManager::cleanTop(
    const string& productName) {
    auto it = heaps.find(productName);
    if (it == heaps.end()) {
        return;
    }
    ProductMinHeap& heap = it->second;
    while (!heap.empty()) {
        Product* product = heap.top();
        if (product->status == "AVAILABLE") {
            break;
        }
        heap.pop();
    }
}
// PEEK BEST
Product* InventoryManager::peekBest(
    const string& productName) {
    cleanTop(productName);
    auto it = heaps.find(productName);
    if (it == heaps.end()) {
        return nullptr;
    }
    return it->second.top();
}
// SELECT BEST
Product* InventoryManager::selectBest(
    const string& productName) {
    cleanTop(productName);
    auto it = heaps.find(productName);
    if (it == heaps.end()) {
        return nullptr;
    }
    Product* best = it->second.pop();
    if (best == nullptr) {
        return nullptr;
    }
    best->status = "RESERVED";
    return best;
}
// RELEASE
bool InventoryManager::releaseProduct(
    const string& id) {
    auto it = productById.find(id);
    if (it == productById.end()) {
        return false;
    }
    Product* product = it->second;
    if (product->status != "RESERVED") {
        return false;
    }
    product->status = "AVAILABLE";
    heaps[product->product_name].push(product);
    return true;
}
// EXPIRE
bool InventoryManager::expireProduct(
    const string& id) {
    auto it = productById.find(id);
    if (it == productById.end()) {
        return false;
    }
    Product* product = it->second;
    if (product->status != "AVAILABLE") {
        return false;
    }
    product->status = "EXPIRED";
    return true;
}
// PRINT PRODUCT
void printProduct(const Product* product) {
    if (product == nullptr) {
        cout << "Khong co san pham phu hop.\n";
        return;
    }
    cout << "ID           : "<< product->id << '\n';
    cout << "Product name : "<< product->product_name << '\n';
    cout << "Made date    : "<< product->made_date << '\n';
    cout << "Arrived time : "<< product->arrived_time << '\n';
    cout << "Best by date : "<< product->best_by_date << '\n';
    cout << "Status       : "<< product->status << '\n';
}
