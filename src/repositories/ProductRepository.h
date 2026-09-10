#pragma once

#include "../models/Product.h"
#include <drogon/drogon.h>

class ProductRepository
{
public:

    static void createProduct(
        const Product& product,
        std::function<void(long long)> onSuccess,
        std::function<void(const std::string&)> onError
    );

    static void getAllProducts(
        std::function<void(const drogon::orm::Result&)> onSuccess,
        std::function<void(const std::string&)> onError
    );

    static void getProductById(
    long long id,
    std::function<void(const drogon::orm::Result&)> onSuccess,
    std::function<void(const std::string&)> onError
    );
};
