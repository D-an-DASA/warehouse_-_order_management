#ifndef SEARCH_CORE_H
#define SEARCH_CORE_H

#include <cstddef>
#include <string>
#include <vector>

#include "Product.h"
#include "hashtable/hashtable.h"
#include "trie/trie.h"
#include "LRU_Cache/LRU_Cache.h"

class SearchCore {
private:
    // Nguồn lưu Product chính, tìm chính xác bằng ID
    HashTable productTable;

    // Mục lục tên sản phẩm, dùng để tìm theo tiền tố
    Trie productNameTrie;

    // Lưu những thao tác hoặc sản phẩm gần đây
    LRU_Cache recentActions;

    // Số sẽ được dùng để tạo ID sản phẩm tiếp theo
    unsigned long long nextProductNumber = 1;

    // Số chữ số trong ID, ví dụ P00001 có độ rộng 5
    int idWidth = 5;

    void updateIdCounter(
        const std::string& id
    );  // Cập nhật số thứ tự ID khi đọc dữ liệu CSV

public:
    SearchCore() = default;

    // Không cho phép sao chép SearchCore
    SearchCore(const SearchCore&) = delete;

    SearchCore& operator=(const SearchCore&) = delete;

    // Đọc dữ liệu ban đầu từ file CSV
    bool loadCSV(const std::string& filename);

    // Thêm sản phẩm và ghi ID được sinh ra vào product
    bool addProduct(Product& product);

    // Xóa sản phẩm theo ID
    bool deleteProduct(const std::string& id);

    // Tìm chính xác theo ID, nếu không có thì tìm tên theo tiền tố
    std::vector<Product> search(
        const std::string& query,
        std::size_t limit = 50
    );

    // Gợi ý tên sản phẩm theo tiền tố
    std::vector<std::string> autocomplete(
        const std::string& prefix,
        std::size_t limit = 10
    );

    // Lấy danh sách thao tác gần đây
    std::vector<CacheItem> getRecent() const;

    // Lấy tổng số sản phẩm trong HashTable
    int size() const;
};

#endif
