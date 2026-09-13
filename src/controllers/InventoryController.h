#pragma once

#include <drogon/drogon.h>

class InventoryController
{
public:
    static void receiveInventory(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    static void getInventory(
        const drogon::HttpRequestPtr& request,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};
