#include <drogon/drogon.h>
#include "controllers/ProductController.h"

int main()
{
    drogon::app().loadConfigFile("../config/config.json");

    // Health check
    drogon::app().registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            Json::Value response;
            response["status"] = "ok";

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(response);

            callback(resp);
        },
        {drogon::Get}
    );

    // Create product
    drogon::app().registerHandler(
        "/products",
        &ProductController::createProduct,
        {drogon::Post}
    );

    //get product
    drogon::app().registerHandler(
    "/products",
    &ProductController::getAllProducts,
    {drogon::Get}
    );

    //get product by id
    drogon::app().registerHandler(
    "/products/{id}",
    [](const drogon::HttpRequestPtr& request,
       std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        ProductController::getProductById(
            request,
            std::move(callback)
        );
    },
    {drogon::Get}
);

    drogon::app().run();

    return 0;
}
