#include "InventoryController.h"

#include "../models/Inventory.h"
#include "../services/InventoryService.h"
#include "../repositories/InventoryRepository.h"

void InventoryController::receiveInventory(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = request->getJsonObject();

    if (!json ||
        !json->isMember("warehouse_id") ||
        !json->isMember("product_id") ||
        !json->isMember("quantity"))
    {
        Json::Value responseJson;
        responseJson["error"] =
            "warehouse_id, product_id and quantity are required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(responseJson);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    Inventory inventory;

    inventory.warehouseId =
        (*json)["warehouse_id"].asInt64();

    inventory.productId =
        (*json)["product_id"].asInt64();

    inventory.quantity =
        (*json)["quantity"].asInt();

    auto callbackPtr =
        std::make_shared<
            std::function<void(const drogon::HttpResponsePtr&)>>(
                std::move(callback));

    InventoryService::receiveInventory(
        inventory,

        [callbackPtr]()
        {
            Json::Value responseJson;
            responseJson["message"] = "Inventory received";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(responseJson);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value responseJson;
            responseJson["error"] = error;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(responseJson);

            response->setStatusCode(drogon::k400BadRequest);

            (*callbackPtr)(response);
        }
    );
}

void InventoryController::getInventory(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto path = request->getPath();

    auto warehouseIdString = path.substr(std::string("/inventory/").length());
    if (warehouseIdString.empty())
    {
        Json::Value responseJson;
        responseJson["error"] =
            "warehouse_id is required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(responseJson);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    long long warehouseId;

    try
    {
        warehouseId = std::stoll(warehouseIdString);
    }
    catch (...)
    {
        Json::Value responseJson;
        responseJson["error"] =
            "Invalid warehouse ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(responseJson);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    auto callbackPtr =
        std::make_shared<
            std::function<void(const drogon::HttpResponsePtr&)>>(
                std::move(callback));

    InventoryRepository::getInventory(
        warehouseId,

        [callbackPtr](const drogon::orm::Result& result)
        {
            Json::Value inventoryList(Json::arrayValue);

            for (const auto& row : result)
            {
                Json::Value item;

                item["warehouse_id"] =
                    Json::Int64(row["warehouse_id"].as<long long>());

                item["product_id"] =
                    Json::Int64(row["product_id"].as<long long>());

                item["quantity"] =
                    row["quantity"].as<int>();

                item["reserved_quantity"] =
                    row["reserved_quantity"].as<int>();

                item["available_quantity"] =
                    row["quantity"].as<int>() -
                    row["reserved_quantity"].as<int>();

                inventoryList.append(item);
            }

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    inventoryList);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value responseJson;
            responseJson["error"] = error;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(responseJson);

            response->setStatusCode(
                drogon::k500InternalServerError);

            (*callbackPtr)(response);
        }
    );
}
