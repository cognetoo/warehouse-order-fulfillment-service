#include "WarehouseController.h"

#include "../models/Warehouse.h"
#include "../repositories/WarehouseRepository.h"

void WarehouseController::createWarehouse(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = request->getJsonObject();

    if (!json ||
        !json->isMember("name") ||
        !json->isMember("location"))
    {
        Json::Value responseJson;
        responseJson["error"] = "name and location are required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(responseJson);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    Warehouse warehouse;

    warehouse.name = (*json)["name"].asString();
    warehouse.location = (*json)["location"].asString();

    auto callbackPtr =
        std::make_shared<
            std::function<void(const drogon::HttpResponsePtr&)>>(
                std::move(callback));

    WarehouseRepository::createWarehouse(
        warehouse,

        [callbackPtr](long long id)
        {
            Json::Value responseJson;
            responseJson["id"] = Json::Int64(id);
            responseJson["message"] = "Warehouse created";

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

            response->setStatusCode(drogon::k500InternalServerError);

            (*callbackPtr)(response);
        }
    );
}

void WarehouseController::getAllWarehouses(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto callbackPtr =
        std::make_shared<
            std::function<void(const drogon::HttpResponsePtr&)>>(
                std::move(callback));

    WarehouseRepository::getAllWarehouses(
        [callbackPtr](const drogon::orm::Result& result)
        {
            Json::Value warehouses(Json::arrayValue);

            for (const auto& row : result)
            {
                Json::Value warehouse;

                warehouse["id"] =
                    Json::Int64(row["id"].as<long long>());

                warehouse["name"] =
                    row["name"].as<std::string>();

                warehouse["location"] =
                    row["location"].as<std::string>();

                warehouses.append(warehouse);
            }

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(warehouses);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value responseJson;
            responseJson["error"] = error;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(responseJson);

            response->setStatusCode(drogon::k500InternalServerError);

            (*callbackPtr)(response);
        }
    );
}
