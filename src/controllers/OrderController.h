#pragma once

#include <drogon/drogon.h>

class OrderController
{
public:
    static void createOrder(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    static void getOrder(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};
