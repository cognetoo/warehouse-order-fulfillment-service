#include "WarehouseRepository.h"

void WarehouseRepository::createWarehouse(
    const Warehouse& warehouse,
    std::function<void(long long)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "INSERT INTO warehouses (name, location) "
        "VALUES ($1, $2) "
        "RETURNING id",
        [onSuccess](const drogon::orm::Result& result)
        {
            long long id = result[0]["id"].as<long long>();
            onSuccess(id);
        },
        [onError](const drogon::orm::DrogonDbException& exception)
        {
            onError(exception.base().what());
        },
        warehouse.name,
        warehouse.location
    );
}

void WarehouseRepository::getAllWarehouses(
    std::function<void(const drogon::orm::Result&)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT id, name, location, created_at "
        "FROM warehouses "
        "ORDER BY id",
        [onSuccess](const drogon::orm::Result& result)
        {
            onSuccess(result);
        },
        [onError](const drogon::orm::DrogonDbException& exception)
        {
            onError(exception.base().what());
        }
    );
}
