#pragma once

#include <drogon/drogon.h>

class WarehouseController
{
public:
    static void createWarehouse(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    static void getAllWarehouses(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};
