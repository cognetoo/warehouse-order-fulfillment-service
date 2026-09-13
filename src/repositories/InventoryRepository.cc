#include "InventoryRepository.h"

void InventoryRepository::receiveInventory(
    const Inventory& inventory,
    std::function<void()> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "INSERT INTO inventory "
        "(warehouse_id, product_id, quantity, reserved_quantity) "
        "VALUES ($1, $2, $3, 0) "
        "ON CONFLICT (warehouse_id, product_id) "
        "DO UPDATE SET "
        "quantity = inventory.quantity + EXCLUDED.quantity, "
        "updated_at = CURRENT_TIMESTAMP",

        [onSuccess](const drogon::orm::Result&)
        {
            onSuccess();
        },

        [onError](const drogon::orm::DrogonDbException& exception)
        {
            onError(exception.base().what());
        },

        inventory.warehouseId,
        inventory.productId,
        inventory.quantity
    );
}

void InventoryRepository::getInventory(
    long long warehouseId,
    std::function<void(const drogon::orm::Result&)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT warehouse_id, product_id, quantity, "
        "reserved_quantity, updated_at "
        "FROM inventory "
        "WHERE warehouse_id = $1 "
        "ORDER BY product_id",

        [onSuccess](const drogon::orm::Result& result)
        {
            onSuccess(result);
        },

        [onError](const drogon::orm::DrogonDbException& exception)
        {
            onError(exception.base().what());
        },

        warehouseId
    );
}
