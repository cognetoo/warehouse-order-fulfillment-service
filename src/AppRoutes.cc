#include <drogon/drogon.h>

#include "AppRoutes.h"

#include "controllers/ProductController.h"
#include "controllers/WarehouseController.h"
#include "controllers/InventoryController.h"
#include "controllers/OrderController.h"

void registerAppRoutes()
{
    // Health check
    drogon::app().registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            Json::Value responseJson;
            responseJson["status"] = "ok";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(responseJson);

            callback(response);
        },
        {drogon::Get}
    );

    // Product routes
    drogon::app().registerHandler(
        "/products",
        &ProductController::createProduct,
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/products",
        &ProductController::getAllProducts,
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/products/{id}",
        &ProductController::getProductById,
        {drogon::Get}
    );

    // Warehouse routes
    drogon::app().registerHandler(
        "/warehouses",
        &WarehouseController::createWarehouse,
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/warehouses",
        &WarehouseController::getAllWarehouses,
        {drogon::Get}
    );

    // Inventory routes
    drogon::app().registerHandler(
        "/inventory/receive",
        &InventoryController::receiveInventory,
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/inventory/{warehouse_id}",
        &InventoryController::getInventory,
        {drogon::Get}
    );

    // Order routes
    drogon::app().registerHandler(
        "/orders",
        &OrderController::createOrder,
        {drogon::Post}
    );

    drogon::app().registerHandler(
        "/orders/{id}",
        &OrderController::getOrder,
        {drogon::Get}
    );
}
