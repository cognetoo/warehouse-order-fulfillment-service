#include "ProductController.h"
#include "../repositories/ProductRepository.h"
#include "../models/Product.h"

void ProductController::createProduct(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = request->getJsonObject();

    if (!json)
    {
        auto response = drogon::HttpResponse::newHttpResponse();
        response->setStatusCode(drogon::k400BadRequest);
        response->setBody("Invalid JSON");

        callback(response);
        return;
    }

    Product product;

    product.sku = (*json)["sku"].asString();
    product.name = (*json)["name"].asString();
    product.description = (*json)["description"].asString();
    product.price = (*json)["price"].asDouble();

    auto callbackPtr =
        std::make_shared<
            std::function<void(
                const drogon::HttpResponsePtr&)>
        >(std::move(callback));

    ProductRepository::createProduct(
        product,

        [callbackPtr](long long id)
        {
            Json::Value response;

            response["id"] = Json::Int64(id);
            response["message"] =
                "Product created successfully";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k201Created);

            (*callbackPtr)(resp);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value response;
            response["error"] = error;

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            (*callbackPtr)(resp);
        }
    );
}
void ProductController::getAllProducts(
    const drogon::HttpRequestPtr&,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto callbackPtr =
        std::make_shared<
            std::function<void(
                const drogon::HttpResponsePtr&)>
        >(std::move(callback));

    ProductRepository::getAllProducts(

        [callbackPtr](const drogon::orm::Result& result)
        {
            Json::Value products(Json::arrayValue);

            for (const auto& row : result)
            {
                Json::Value product;

                product["id"] =
                    Json::Int64(row["id"].as<long long>());

                product["sku"] =
                    row["sku"].as<std::string>();

                product["name"] =
                    row["name"].as<std::string>();

                product["description"] =
                    row["description"].as<std::string>();

                product["price"] =
                    row["price"].as<double>();

                products.append(product);
            }

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    products);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value response;

            response["error"] = error;

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            (*callbackPtr)(resp);
        }
    );
}
void ProductController::getProductById(
    const drogon::HttpRequestPtr& request,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto path = request->getPath();

    auto idString = path.substr(std::string("/products/").length());

    long long id;

    try
    {
        id = std::stoll(idString);
    }
    catch (...)
    {
        auto response =
            drogon::HttpResponse::newHttpResponse();

        response->setStatusCode(
            drogon::k400BadRequest);

        response->setBody("Invalid product ID");

        callback(response);
        return;
    }

    auto callbackPtr =
        std::make_shared<
            std::function<void(
                const drogon::HttpResponsePtr&)>
        >(std::move(callback));

    ProductRepository::getProductById(
        id,

        [callbackPtr](const drogon::orm::Result& result)
        {
            if (result.empty())
            {
                auto response =
                    drogon::HttpResponse::newHttpResponse();

                response->setStatusCode(
                    drogon::k404NotFound);

                response->setBody("Product not found");

                (*callbackPtr)(response);
                return;
            }

            const auto& row = result[0];

            Json::Value product;

            product["id"] =
                Json::Int64(row["id"].as<long long>());

            product["sku"] =
                row["sku"].as<std::string>();

            product["name"] =
                row["name"].as<std::string>();

            product["description"] =
                row["description"].as<std::string>();

            product["price"] =
                row["price"].as<double>();

            auto response =
                drogon::HttpResponse::newHttpJsonResponse(
                    product);

            (*callbackPtr)(response);
        },

        [callbackPtr](const std::string& error)
        {
            Json::Value response;

            response["error"] = error;

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(
                    response);

            resp->setStatusCode(
                drogon::k500InternalServerError);

            (*callbackPtr)(resp);
        }
    );
}
