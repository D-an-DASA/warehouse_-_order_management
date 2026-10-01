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
    HashTable productTable;
    Trie productNameTrie;
    LRU_Cache recentActions;
    unsigned long long nextProductNumber = 1;
    int idWidth = 5;

    static std::string normalizeText(const std::string& text);
    static std::string normalizeId(const std::string& id);
    static bool isValidProductId(const std::string& id);
    static bool isValidDate(const std::string& value);
    static bool isValidArrivedTime(const std::string& value);
    static bool hasUnsafeCsvCharacters(const std::string& value);

    bool validateProduct(Product& product, std::string& error) const;
    void updateIdCounter(const std::string& id);

public:
    SearchCore() = default;
    SearchCore(const SearchCore&) = delete;
    SearchCore& operator=(const SearchCore&) = delete;

    bool loadProducts(const std::vector<Product>& products, std::string& error);
    bool addProduct(Product& product, std::string& error);
    bool deleteProduct(const std::string& rawId, std::string& error);
    bool reserveProduct(const std::string& rawId, Product& reserved,
                        std::string& error);

    bool findById(const std::string& rawId, Product& product,
                  bool recordRecent = true);

    std::vector<Product> previewPriority(
        const std::string& rawPrefix,
        std::size_t limit = 50
    );

    std::vector<Product> search(
        const std::string& rawQuery,
        std::size_t limit = 50
    );

    std::vector<std::string> autocomplete(
        const std::string& rawPrefix,
        std::size_t limit = 20
    );

    std::vector<CacheItem> getRecent() const;
    std::vector<Product> getAllProducts() const;
    int size() const;
};

#endif
