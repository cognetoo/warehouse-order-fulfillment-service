#pragma once

#include "../models/Order.h"

#include <drogon/drogon.h>
#include <functional>
#include <string>

class OrderRepository
{
public:

    static void createOrderWithReservation(
        const Order& order,
        std::function<void(long long)> onSuccess,
        std::function<void(const std::string&)> onError
    );

    static void getOrder(
        long long orderId,
        std::function<void(const Order&)> onSuccess,
        std::function<void(const std::string&)> onError
    );
};
