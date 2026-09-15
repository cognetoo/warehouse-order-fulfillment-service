#pragma once

#include <string>
#include <vector>

struct OrderItem
{
    long long productId;
    int quantity;
};

struct Order
{
    long long id;
    long long warehouseId;
    std::string customerName;
    std::string status;
    std::vector<OrderItem> items;
};
