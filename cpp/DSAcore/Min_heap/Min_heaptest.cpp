#include <iostream>
#include <string>
#include "Min_heap.h"

using namespace std;

int main() {
    InventoryManager inventory;
    string filename = "product_inventory_10 000.csv";
    // LOAD DATASET
    if (!inventory.loadCSV(filename)) {
        return 1;
    }
    cout << "========================================\n";
    cout << "       PRODUCT INVENTORY SYSTEM\n";
    cout << "========================================\n";
    cout << "Tong so san pham : "<< inventory.size() << '\n';
    cout << "AVAILABLE        : "<< inventory.countAvailable() << '\n';
    while (true) {
        cout << "\n";
        cout << "0. Thoat\n";
        cout << "1. Xem san pham uu tien nhat\n";
        cout << "2. Chon san pham uu tien nhat\n";
        cout << "3. Tra lai san pham RESERVED\n";
        cout << "4. Danh dau san pham EXPIRED\n";
        int choice;
        cout << "Lua chon: ";
        cin >> choice;
        cin.ignore();
        // TEST PEEK
        if (choice == 1) {
            string productName;
            cout << "Nhap product_name: ";
            getline(cin, productName);
            Product* best =inventory.peekBest(productName);
            cout << "\nSan pham uu tien nhat:\n";
            printProduct(best);
        }
        // TEST SELECT
        else if (choice == 2) {
            string productName;
            cout << "Nhap product_name: ";
            getline(cin, productName);
            Product* best =inventory.selectBest(productName);
            cout << "\nSan pham duoc chon:\n";
            printProduct(best);
        }
        // TEST RELEASE
        else if (choice == 3) {
            string id;
            cout << "Nhap ID: ";
            getline(cin, id);
            if (inventory.releaseProduct(id)) {
                cout << "Da chuyen "<< id<< " ve AVAILABLE.\n";
            }
            else {
                cout << "Khong the release san pham.\n";
            }
        }
        // TEST EXPIRE
        else if (choice == 4) {
            string id;
            cout << "Nhap ID: ";
            getline(cin, id);
            if (inventory.expireProduct(id)) {
                cout << "Da chuyen "<< id<< " sang EXPIRED.\n";
            }
            else {
                cout << "Khong the expire san pham.\n";
            }
        }
        // THOÁT
        else if (choice == 0) {
            break;
        }
        else {
            cout << "Lua chon khong hop le.\n";
        }
    }
    return 0;
}
