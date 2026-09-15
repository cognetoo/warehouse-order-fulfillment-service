#pragma once

#include "../models/Order.h"

#include <functional>
#include <string>

class OrderService
{
public:
    static void createOrder(
        const Order& order,
        std::function<void(long long)> onSuccess,
        std::function<void(const std::string&)> onError
    );
};
