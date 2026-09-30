#ifndef SEARCH_CORE_H
#define SEARCH_CORE_H

#include <cstddef>
#include <string>
#include <vector>

#include "Product.h"
#include "hashtable/hashtable.h"
#include "trie/trie.h"
#include "LRU_Cache/LRU_Cache.h"

// Kết quả trả về sau khi tìm kiếm
struct SearchResult {
    // Danh sách sản phẩm thực sự được trả về
    std::vector<Product> products;

    // Tổng số sản phẩm tìm thấy trước khi giới hạn kết quả
    std::size_t total = 0;
};

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

    // Nhóm hàm chuẩn hóa chuỗi
    static std::string trim(const std::string& value);

    static std::string collapseSpaces(
        const std::string& value
    );

    static std::string normalizeName(
        const std::string& value
    );

    static std::string normalizeId(
        const std::string& value
    );

    // Nhóm hàm kiểm tra dữ liệu
    static bool isProductId(
        const std::string& value
    );

    static bool isValidDate(
        const std::string& value
    );

    static bool normalizeArrivedTime(
        std::string& arrivedTime,
        std::string& error
    );

    // Kiểm tra và chuẩn hóa một Product
    bool validateProduct(
        Product& product,
        std::string& error
    ) const;

    // Nhóm hàm hỗ trợ ID và CSV
    static std::vector<std::string> splitCsv(
        const std::string& line
    );  // Tách một dòng CSV thành nhiều cột

    void updateIdCounter(
        const std::string& id
    );  // Cập nhật số thứ tự ID khi đọc dữ liệu CSV

    std::string generateId();   // Sinh ID mới cho sản phẩm

public:
    SearchCore() = default;

    // Không cho phép sao chép SearchCore
    SearchCore(const SearchCore&) = delete;

    SearchCore& operator=(const SearchCore&) = delete;

    // Đọc dữ liệu ban đầu từ file CSV
    bool loadCSV(
        const std::string& filename,
        std::string& error
    );

    // Thêm sản phẩm và sinh ID trong SearchCore
    bool addProduct(
        Product input,
        Product& created,
        std::string& error
    );

    // Xóa sản phẩm theo ID
    bool deleteProduct(
        const std::string& rawId,
        std::string& error
    );

    // Tự phân biệt tìm theo ID hay tìm theo tên
    SearchResult search(
        const std::string& rawQuery,
        std::size_t limit = 50,
        bool recordRecent = true
    );

    // Gợi ý tên sản phẩm theo tiền tố
    std::vector<std::string> autocomplete(
        const std::string& rawPrefix,
        std::size_t limit = 10
    );

    // Lấy danh sách thao tác gần đây
    std::vector<CacheItem> getRecent() const;

    // Lấy tổng số sản phẩm trong HashTable
    int size() const;
};

#endif
