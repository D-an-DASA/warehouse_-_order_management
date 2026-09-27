#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
using namespace std;
enum class ProductStatus {
    AVAILABLE,
    RESERVED,
    EXPIRED
};
struct Product{
    string id;
    string productName;
    string madeDate;
    string arrivedTime;
    string bestByDate;
    ProductStatus status;
};
ProductStatus stringToStatus(const string& status){   //chuyển status từ string
    if(status== "AVAILABLE") return ProductStatus::AVAILABLE;
    if(status== "RESERVED") return ProductStatus::RESERVED;
    return ProductStatus::EXPIRED; 
}
string statusToString(ProductStatus status){            //chuyển status sang string
    if(status== ProductStatus::AVAILABLE) return "AVAILABLE";
    if(status== ProductStatus::RESERVED) return "RESERVED";
    return "EXPIRED";
}
vector<string> splistCSV(const string& line){       //tách dấu ,
    vector<string> result;
    string value;
    stringstream ss(line);
    while (getline(ss,value,',')){
        result.push_back(value);
    } 
    return result;
}
//MC2
bool uutien(const Product* a,const Product* b){
    if(a->bestByDate!= b->bestByDate){          //date nhỏ hơn
        return a->bestByDate< b->bestByDate;
    }
    if (a->arrivedTime!=b->arrivedTime){        //nếu = so arrived time nhỏ hơn
        return a->arrivedTime< b->arrivedTime;
    }
    return a->id<b->id;                            //nếu = thì so id nhỏ hơn
}
class ProductMinHeap{
private:
    vector<Product*> heap;
    int root(int index){
        return (index-1)/2;
    } 
    int leftChild(int index){
        return index *2+1;
    }
    int rightChild(int index){
        return index*2+2;
    }
    void swapNode(int a,int b){
        Product* temp=heap[a];
        heap[a]=heap[b];
        heap[b]=temp;
    }
    void heapifyUp(int index){
        while(index>0){
            int p=root(index);
            if (uutien(heap[index],heap[p]))
            {
                swapNode(index,p);
                index=p;
            }
            else{
                break;
            }
        }
    }
    void heapifyDown(int index){
        int n=static_cast<int>(heap.size());
        while(true){
            int left=leftChild(index);
            int right=rightChild(index);
            int smallest=index;
            if(left<n&&uutien(heap[left],heap[smallest])){
                smallest=left;
            }
            if(right<n&&uutien(heap[right],heap[smallest])){
                smallest=right;
            }
            if(smallest==index) break;
            swapNode(index,smallest);
            index=smallest;
        }
    }
public:
    bool empty() const{
        return heap.empty();
    }
    int size() const{
        return static_cast<int>(heap.size());
    }
    void push(Product* product){
        heap.push_back(product);
        heapifyUp(static_cast<int>(heap.size())-1);
    }
    Product* top() const{
        if(heap.empty()){
            return nullptr;
        }
        return heap[0];
    }
    Product* pop() {
        if(heap.empty()){
            return nullptr;
        }
        Product* result=heap[0];
        heap[0]=heap.back();
        heap.pop_back();
        if(!heap.empty()){
            heapifyDown(0);
        }
        return result;
    }
};
class InventoryManager {
private:
    vector<Product> products;
    // MC2:
    unordered_map<string, ProductMinHeap> heaps;
    // Tìm Product theo ID.
    unordered_map<string, Product*> productById;
public:
    // LOAD DATASET
    bool loadCSV(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Khong the mo file: "
                 << filename << '\n';
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
                cerr << "Dong CSV khong hop le: "
                     << line << '\n';
                continue;
            }
            Product product;
            product.id = fields[0];
            product.productName = fields[1];
            product.madeDate = fields[2];
            product.arrivedTime = fields[3];
            product.bestByDate = fields[4];
            product.status =stringToStatus(fields[5]);
            products.push_back(product);
        }
        file.close();
        for (Product& product : products) {
            productById[product.id] = &product;
            // MC2 chỉ quan tâm AVAILABLE
            if (product.status ==ProductStatus::AVAILABLE) {
                heaps[product.productName].push(&product);
            }
        }
        return true;
    }
    // SỐ LƯỢNG SẢN PHẨM
    int size() const {
        return static_cast<int>(
            products.size()
        );
    }
    int countAvailable() const {        // ĐẾM AVAILABLE
        int count = 0;
        for (const Product& product : products) {
            if (product.status ==ProductStatus::AVAILABLE) {
                count++;
            }
        }
        return count;
    }
    // CLEAN TOP
    // Nếu product ở root nhưng không còn AVAILABLE thì loại bỏ khỏi heap.
    void cleanTop(const string& productName) {
        auto it = heaps.find(productName);
        if (it == heaps.end()) {
            return;
        }
        ProductMinHeap& heap = it->second;
        while (!heap.empty()) {
            Product* product = heap.top();
            if (product->status ==ProductStatus::AVAILABLE) {
                break;
            }
            heap.pop();
        }
    }
    // MC2 - XEM SẢN PHẨM TỐT NHẤT
    // Không thay đổi status.
    Product* peekBest(const string& productName) {
        cleanTop(productName);
        auto it = heaps.find(productName);
        if (it == heaps.end()) {
            return nullptr;
        }
        return it->second.top();
    }
    // MC2 - CHỌN SẢN PHẨM TỐT NHẤT
    // Sau khi chọn:AVAILABLE -> RESERVED
    Product* selectBest(const string& productName) {
        cleanTop(productName);
        auto it = heaps.find(productName);
        if (it == heaps.end()) {
            return nullptr;
        }
        ProductMinHeap& heap = it->second;
        Product* best = heap.pop();
        if (best == nullptr) {
            return nullptr;
        }
        // Sản phẩm đã được chọn
        best->status =ProductStatus::RESERVED;      // nếu không cần chỉnh thì đổi sang AVAILABLE
        return best;
    }
    // RESERVED -> AVAILABLE
    // Dùng khi sản phẩm được trả lại trạng thái AVAILABLE.
    bool releaseProduct(const string& id) {
        auto it = productById.find(id);
        if (it == productById.end()) {
            return false;
        }
        Product* product = it->second;
        if (product->status !=ProductStatus::RESERVED) {
            return false;
        }
        product->status =ProductStatus::AVAILABLE;
        heaps[product->productName].push(product);
        return true;
    }
    // AVAILABLE -> EXPIRED
    // Không cần tìm và xóa trực tiếp khỏi heap.
    // cleanTop() sẽ xử lý khi nó lên root.
    bool expireProduct(const string& id) {
        auto it = productById.find(id);
        if (it == productById.end()) {
            return false;
        }
        Product* product = it->second;
        if (product->status !=ProductStatus::AVAILABLE) {
            return false;
        }
        product->status =ProductStatus::EXPIRED;
        return true;
    }
};
// IN THÔNG TIN PRODUCT
void printProduct(const Product* product) {
    if (product == nullptr) {
        cout << "Khong co san pham phu hop.\n";
        return;
    }
    cout << "----------------------------------------\n";
    cout << "ID           : "<< product->id << '\n';
    cout << "Product name : "<< product->productName << '\n';
    cout << "Made date    : "<< product->madeDate << '\n';
    cout << "Arrived time : "<< product->arrivedTime << '\n';
    cout << "Best by date : "<< product->bestByDate << '\n';
    cout << "Status       : "<< statusToString(product->status)<< '\n';
    cout << "----------------------------------------\n";
}
// MAIN
int main(){
     InventoryManager inventory;
    // ĐƯỜNG DẪN DATASET
    string filename ="product_inventory_10 000.csv";
    if (!inventory.loadCSV(filename)) {     //load data
        return 1;
    }
    cout << "========================================\n";
    cout << "       PRODUCT INVENTORY SYSTEM\n";
    cout << "========================================\n";
    cout << "Tong so san pham : "<< inventory.size()<< '\n';
    cout << "AVAILABLE        : "<< inventory.countAvailable()<< '\n';
    while (true) {
        cout << "\n";
        cout << "========================================\n";
        cout << "MC2 - CHON SAN PHAM UU TIEN NHAT\n";
        cout << "========================================\n";
        cout << "0. Thoat\n";
        cout << "1. Xem san pham uu tien nhat\n";
        cout << "2. Chon san pham uu tien nhat\n";
        cout << "3. Tra lai san pham RESERVED\n";
        cout << "4. Danh dau san pham EXPIRED\n";
        cout << "Lua chon: ";
        int choice;
        cin >> choice;
        cin.ignore();
        if (choice == 0) {      //Thoát
            break;
        }
        if (choice == 1) {      //peek
            string productName;
            cout << "Nhap product_name: ";
            getline(cin,productName);
            Product* best =inventory.peekBest(productName);
            cout << "\nSan pham uu tien nhat:\n";
            printProduct(best);
        }
        else if (choice == 2) {     //select
            string productName;
            cout << "Nhap product_name: ";
            getline(cin,productName);
            Product* best =inventory.selectBest(productName);
            cout << "\nSan pham duoc chon:\n";
            printProduct(best);
        }
        else if (choice == 3) {     //release
            string id;
            cout << "Nhap ID: ";
            getline(cin, id);
            if (inventory.releaseProduct(id)) {
                cout << "Da chuyen san pham "<<id<< " ve AVAILABLE.\n";
            }
            else {
                cout << "Khong the release san pham.\n";
            }
        }
        else if (choice == 4) {     //expire
            string id;
            cout << "Nhap ID: ";
            getline(cin, id);
            if (inventory.expireProduct(id)) {
                cout << "Da chuyen san pham "<<id<< " sang EXPIRED.\n";
            }
            else {
                cout << "Khong the expire san pham.\n";
            }
        }
        else {
            cout << "Lua chon khong hop le.\n";
        }
    }
    return 0;
}