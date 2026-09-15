#include "OrderController.h"

#include "../models/Order.h"
#include "../services/OrderService.h"
#include "../repositories/OrderRepository.h"

void OrderController::createOrder(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = request->getJsonObject();

    if (!json ||
        !json->isMember("customer_name") ||
        !json->isMember("warehouse_id") ||
        !json->isMember("items"))
    {
        Json::Value responseJson;
        responseJson["error"] =
            "customer_name, warehouse_id and items are required";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                responseJson);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    Order order;

    // ------------------------------------------------------------
    // Customer
    // ------------------------------------------------------------

    order.customerName =
        (*json)["customer_name"].asString();

    // ------------------------------------------------------------
    // Warehouse
    // ------------------------------------------------------------

    order.warehouseId =
        (*json)["warehouse_id"].asInt64();

    // ------------------------------------------------------------
    // Items
    // ------------------------------------------------------------

    const auto& items = (*json)["items"];

    if (!items.isArray())
    {
        Json::Value responseJson;
        responseJson["error"] =
            "items must be an array";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                responseJson);

        response->setStatusCode(drogon::k400BadRequest);
        callback(response);
        return;
    }

    for (const auto& itemJson : items)
    {
        if (!itemJson.isMember("product_id") ||
            !itemJson.isMember("quantity"))
        {
            Json::Value responseJson;
            responseJson["error"] =
                "Each item requires product_id and quantity";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson);

            response->setStatusCode(drogon::k400BadRequest);
            callback(response);
            return;
        }

        OrderItem item;

        item.productId =
            itemJson["product_id"].asInt64();

        item.quantity =
            itemJson["quantity"].asInt();

        order.items.push_back(item);
    }

    // ------------------------------------------------------------
    // Send validated Order to Service
    // ------------------------------------------------------------

    auto callbackPtr =
        std::make_shared<
            std::function<void(const drogon::HttpResponsePtr&)>>(
                std::move(callback));

    OrderService::createOrder(
        order,

        [callbackPtr](long long orderId)
        {
            Json::Value responseJson;

            responseJson["id"] =
                Json::Int64(orderId);

            responseJson["message"] =
                "Order created";

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value responseJson;
            responseJson["error"] = error;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson);

            response->setStatusCode(
                drogon::k400BadRequest);

            (*callbackPtr)(response);
        }
    );
}
void OrderController::getOrder(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto callbackPtr =
        std::make_shared<
            std::function<void(const drogon::HttpResponsePtr&)>>(
                std::move(callback));

    // Extract ID from /orders/{id}
    auto path = request->getPath();

    const std::string prefix = "/orders/";

    if (path.rfind(prefix, 0) != 0)
    {
        Json::Value responseJson;
        responseJson["error"] = "Invalid order path";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                responseJson);

        response->setStatusCode(drogon::k400BadRequest);

        (*callbackPtr)(response);
        return;
    }

    auto idString = path.substr(prefix.length());

    long long orderId;

    try
    {
        orderId = std::stoll(idString);
    }
    catch (...)
    {
        Json::Value responseJson;
        responseJson["error"] = "Invalid order ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                responseJson);

        response->setStatusCode(drogon::k400BadRequest);

        (*callbackPtr)(response);
        return;
    }

    if (orderId <= 0)
    {
        Json::Value responseJson;
        responseJson["error"] = "Invalid order ID";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(
                responseJson);

        response->setStatusCode(drogon::k400BadRequest);

        (*callbackPtr)(response);
        return;
    }

    // Repository now returns an Order object,
    // not a raw PostgreSQL Result.
    OrderRepository::getOrder(
        orderId,

        [callbackPtr](const Order& order)
        {
            Json::Value orderJson;

            orderJson["id"] =
                Json::Int64(order.id);

            orderJson["warehouse_id"] =
                Json::Int64(order.warehouseId);

            orderJson["customer_name"] =
                order.customerName;

            orderJson["status"] =
                order.status;

            Json::Value items(Json::arrayValue);

            for (const auto& orderItem : order.items)
            {
                Json::Value item;

                item["product_id"] =
                    Json::Int64(orderItem.productId);

                item["quantity"] =
                    orderItem.quantity;

                items.append(item);
            }

            orderJson["items"] = items;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    orderJson);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value responseJson;
            responseJson["error"] = error;

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    responseJson);

            response->setStatusCode(drogon::k404NotFound);

            (*callbackPtr)(response);
        });
}
