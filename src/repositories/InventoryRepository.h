#pragma once

#include "../models/Inventory.h"

#include <drogon/drogon.h>

class InventoryRepository
{
public:
    static void receiveInventory(
        const Inventory& inventory,
        std::function<void()> onSuccess,
        std::function<void(const std::string&)> onError
    );

    static void getInventory(
        long long warehouseId,
        std::function<void(const drogon::orm::Result&)> onSuccess,
        std::function<void(const std::string&)> onError
    );
};
