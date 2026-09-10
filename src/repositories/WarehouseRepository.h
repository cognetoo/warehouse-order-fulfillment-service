#pragma once

#include "../models/Warehouse.h"

#include <drogon/drogon.h>

class WarehouseRepository
{
public:
    static void createWarehouse(
        const Warehouse& warehouse,
        std::function<void(long long)> onSuccess,
        std::function<void(const std::string&)> onError
    );

    static void getAllWarehouses(
        std::function<void(const drogon::orm::Result&)> onSuccess,
        std::function<void(const std::string&)> onError
    );
};
