#pragma once

#include "../models/Inventory.h"

#include <functional>
#include <string>

class InventoryService
{
public:
    static void receiveInventory(
        const Inventory& inventory,
        std::function<void()> onSuccess,
        std::function<void(const std::string&)> onError
    );
};
