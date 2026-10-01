#ifndef CSV_PRODUCT_REPOSITORY_H
#define CSV_PRODUCT_REPOSITORY_H

#include <string>
#include <vector>

#include "../DSAcore/Product.h"

class CsvProductRepository {
public:
    bool load(const std::string& filename,
              std::vector<Product>& products,
              std::string& error) const;

    bool save(const std::string& filename,
              const std::vector<Product>& products,
              std::string& error) const;
};

#endif
