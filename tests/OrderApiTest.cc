#include <gtest/gtest.h>

#include <drogon/drogon.h>
#include <drogon/HttpClient.h>

#include <chrono>
#include <future>
#include <thread>
#include <string>

#include "../src/AppRoutes.h"

class OrderApiTest : public ::testing::Test
{
protected:
    static std::thread appThread;
    static drogon::HttpClientPtr client;

    static void SetUpTestSuite()
    {
        drogon::app().loadConfigFile(
            "tests/config/config.json");

        registerAppRoutes();

        appThread = std::thread([]()
        {
            drogon::app().run();
        });

        for (int i = 0; i < 50; ++i)
        {
            if (drogon::app().hasDbClient("default"))
            {
                break;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
        }

        ASSERT_TRUE(
            drogon::app().hasDbClient("default"));

        client =
            drogon::HttpClient::newHttpClient(
                "http://127.0.0.1:48174");
    }

    static void TearDownTestSuite()
    {
        drogon::app().quit();

        if (appThread.joinable())
        {
            appThread.join();
        }
    }

    static drogon::HttpResponsePtr postJson(
        const std::string& path,
        const Json::Value& body)
    {
        auto request =
            drogon::HttpRequest::newHttpJsonRequest(body);

        request->setMethod(drogon::Post);
        request->setPath(path);

        std::promise<drogon::HttpResponsePtr> promise;
        auto future = promise.get_future();

        client->sendRequest(
            request,
            [&promise](
                drogon::ReqResult result,
                const drogon::HttpResponsePtr& response)
            {
                if (result == drogon::ReqResult::Ok)
                {
                    promise.set_value(response);
                }
                else
                {
                    promise.set_value(nullptr);
                }
            });

        return future.get();
    }

    static drogon::HttpResponsePtr get(
        const std::string& path)
    {
        auto request =
            drogon::HttpRequest::newHttpRequest();

        request->setMethod(drogon::Get);
        request->setPath(path);

        std::promise<drogon::HttpResponsePtr> promise;
        auto future = promise.get_future();

        client->sendRequest(
            request,
            [&promise](
                drogon::ReqResult result,
                const drogon::HttpResponsePtr& response)
            {
                if (result == drogon::ReqResult::Ok)
                {
                    promise.set_value(response);
                }
                else
                {
                    promise.set_value(nullptr);
                }
            });

        return future.get();
    }
};

std::thread OrderApiTest::appThread;
drogon::HttpClientPtr OrderApiTest::client;


// ---------------------------------------------------------
// API validation smoke test
// ---------------------------------------------------------

TEST_F(OrderApiTest, RejectsEmptyCustomerName)
{
    Json::Value body;

    body["customer_name"] = "";
    body["warehouse_id"] = 1;

    Json::Value item;
    item["product_id"] = 1;
    item["quantity"] = 1;

    body["items"] = Json::Value(Json::arrayValue);
    body["items"].append(item);

    auto response =
        postJson("/orders", body);

    ASSERT_NE(response, nullptr);

    EXPECT_EQ(
        response->getStatusCode(),
        drogon::k400BadRequest);
}


// ---------------------------------------------------------
// Full API workflow
// ---------------------------------------------------------

TEST_F(OrderApiTest, CreatesOrderSuccessfully)
{
    // -----------------------------------------------------
    // 1. Create product
    // -----------------------------------------------------

    Json::Value productBody;

    // Unique SKU allows repeated executions of the test.
    auto uniqueSuffix =
        std::to_string(
            std::chrono::steady_clock::now()
                .time_since_epoch()
                .count());

    productBody["sku"] =
        "API-TEST-" + uniqueSuffix;

    productBody["name"] =
        "API Test Product";

    productBody["description"] =
        "Product created by API integration test";

    productBody["price"] = 1000.0;

    auto productResponse =
        postJson("/products", productBody);

    ASSERT_NE(productResponse, nullptr);

    ASSERT_EQ(
        productResponse->getStatusCode(),
        drogon::k201Created);

    auto productJson =
        productResponse->getJsonObject();

    ASSERT_NE(productJson, nullptr);
    ASSERT_TRUE(productJson->isMember("id"));

    long long productId =
        (*productJson)["id"].asInt64();

    ASSERT_GT(productId, 0);


    // -----------------------------------------------------
    // 2. Create warehouse
    // -----------------------------------------------------

    Json::Value warehouseBody;

    warehouseBody["name"] =
        "API Test Warehouse";

    warehouseBody["location"] =
        "Bengaluru";

    auto warehouseResponse =
        postJson("/warehouses", warehouseBody);

    ASSERT_NE(warehouseResponse, nullptr);

    ASSERT_EQ(
        warehouseResponse->getStatusCode(),
        drogon::k200OK);

    auto warehouseJson =
        warehouseResponse->getJsonObject();

    ASSERT_NE(warehouseJson, nullptr);
    ASSERT_TRUE(warehouseJson->isMember("id"));

    long long warehouseId =
        (*warehouseJson)["id"].asInt64();

    ASSERT_GT(warehouseId, 0);


    // -----------------------------------------------------
    // 3. Receive inventory
    // -----------------------------------------------------

    Json::Value inventoryBody;

    inventoryBody["warehouse_id"] =
        Json::Int64(warehouseId);

    inventoryBody["product_id"] =
        Json::Int64(productId);

    inventoryBody["quantity"] = 50;

    auto inventoryResponse =
        postJson(
            "/inventory/receive",
            inventoryBody);

    ASSERT_NE(inventoryResponse, nullptr);

    ASSERT_EQ(
        inventoryResponse->getStatusCode(),
        drogon::k200OK);

    auto inventoryJson =
        inventoryResponse->getJsonObject();

    ASSERT_NE(inventoryJson, nullptr);

    EXPECT_EQ(
        (*inventoryJson)["message"].asString(),
        "Inventory received");


    // -----------------------------------------------------
    // 4. Create order
    // -----------------------------------------------------

    Json::Value orderBody;

    orderBody["customer_name"] =
        "API Integration Customer";

    orderBody["warehouse_id"] =
        Json::Int64(warehouseId);

    Json::Value orderItem;

    orderItem["product_id"] =
        Json::Int64(productId);

    orderItem["quantity"] = 2;

    orderBody["items"] =
        Json::Value(Json::arrayValue);

    orderBody["items"].append(orderItem);

    auto orderResponse =
        postJson("/orders", orderBody);

    ASSERT_NE(orderResponse, nullptr);

    ASSERT_EQ(
        orderResponse->getStatusCode(),
        drogon::k200OK);

    auto orderJson =
        orderResponse->getJsonObject();

    ASSERT_NE(orderJson, nullptr);

    ASSERT_TRUE(
        orderJson->isMember("id"));

    EXPECT_EQ(
        (*orderJson)["message"].asString(),
        "Order created");

    long long orderId =
        (*orderJson)["id"].asInt64();

    ASSERT_GT(orderId, 0);


    // -----------------------------------------------------
    // 5. Retrieve order through HTTP
    // -----------------------------------------------------

    auto getOrderResponse =
        get(
            "/orders/" +
            std::to_string(orderId));

    ASSERT_NE(getOrderResponse, nullptr);

    ASSERT_EQ(
        getOrderResponse->getStatusCode(),
        drogon::k200OK);

    auto retrievedOrder =
        getOrderResponse->getJsonObject();

    ASSERT_NE(retrievedOrder, nullptr);


    // -----------------------------------------------------
    // 6. Verify API response
    // -----------------------------------------------------

    EXPECT_EQ(
        (*retrievedOrder)["id"].asInt64(),
        orderId);

    EXPECT_EQ(
        (*retrievedOrder)["warehouse_id"].asInt64(),
        warehouseId);

    EXPECT_EQ(
        (*retrievedOrder)["customer_name"].asString(),
        "API Integration Customer");

    EXPECT_EQ(
        (*retrievedOrder)["status"].asString(),
        "CONFIRMED");

    ASSERT_TRUE(
        (*retrievedOrder)["items"].isArray());

    ASSERT_EQ(
        (*retrievedOrder)["items"].size(),
        1);

    EXPECT_EQ(
        (*retrievedOrder)["items"][0]
            ["product_id"].asInt64(),
        productId);

    EXPECT_EQ(
        (*retrievedOrder)["items"][0]
            ["quantity"].asInt(),
        2);


    // -----------------------------------------------------
    // 7. Verify inventory through HTTP
    // -----------------------------------------------------

    auto inventoryCheckResponse =
        get(
            "/inventory/" +
            std::to_string(warehouseId));

    ASSERT_NE(
        inventoryCheckResponse,
        nullptr);

    ASSERT_EQ(
        inventoryCheckResponse->getStatusCode(),
        drogon::k200OK);

    auto inventoryList =
        inventoryCheckResponse->getJsonObject();

    ASSERT_NE(inventoryList, nullptr);
    ASSERT_TRUE(inventoryList->isArray());
    ASSERT_EQ(inventoryList->size(), 1);

    EXPECT_EQ(
        (*inventoryList)[0]
            ["quantity"].asInt(),
        50);

    EXPECT_EQ(
        (*inventoryList)[0]
            ["reserved_quantity"].asInt(),
        2);

    EXPECT_EQ(
        (*inventoryList)[0]
            ["available_quantity"].asInt(),
        48);
}
TEST_F(OrderApiTest, Returns404ForMissingOrder)
{
    auto response =
        get("/orders/999999999");

    ASSERT_NE(response, nullptr);

    EXPECT_EQ(
        response->getStatusCode(),
        drogon::k404NotFound);

    auto json =
        response->getJsonObject();

    ASSERT_NE(json, nullptr);

    ASSERT_TRUE(
        json->isMember("error"));

    EXPECT_EQ(
        (*json)["error"].asString(),
        "Order not found.");
}
TEST_F(OrderApiTest, RejectsInsufficientInventory)
{
    // -----------------------------------------------------
    // 1. Create a product
    // -----------------------------------------------------

    Json::Value productBody;

    auto uniqueSuffix =
        std::to_string(
            std::chrono::steady_clock::now()
                .time_since_epoch()
                .count());

    productBody["sku"] =
        "API-INSUFFICIENT-" + uniqueSuffix;

    productBody["name"] =
        "Insufficient Inventory Test Product";

    productBody["description"] =
        "Product used for API failure test";

    productBody["price"] = 500.0;

    auto productResponse =
        postJson("/products", productBody);

    ASSERT_NE(productResponse, nullptr);

    ASSERT_EQ(
        productResponse->getStatusCode(),
        drogon::k201Created);

    auto productJson =
        productResponse->getJsonObject();

    ASSERT_NE(productJson, nullptr);
    ASSERT_TRUE(productJson->isMember("id"));

    long long productId =
        (*productJson)["id"].asInt64();

    ASSERT_GT(productId, 0);


    // -----------------------------------------------------
    // 2. Create a warehouse
    // -----------------------------------------------------

    Json::Value warehouseBody;

    warehouseBody["name"] =
        "API Insufficient Test Warehouse";

    warehouseBody["location"] =
        "Bengaluru";

    auto warehouseResponse =
        postJson("/warehouses", warehouseBody);

    ASSERT_NE(warehouseResponse, nullptr);

    ASSERT_EQ(
        warehouseResponse->getStatusCode(),
        drogon::k200OK);

    auto warehouseJson =
        warehouseResponse->getJsonObject();

    ASSERT_NE(warehouseJson, nullptr);
    ASSERT_TRUE(warehouseJson->isMember("id"));

    long long warehouseId =
        (*warehouseJson)["id"].asInt64();

    ASSERT_GT(warehouseId, 0);


    // -----------------------------------------------------
    // 3. Receive only 5 units
    // -----------------------------------------------------

    Json::Value inventoryBody;

    inventoryBody["warehouse_id"] =
        Json::Int64(warehouseId);

    inventoryBody["product_id"] =
        Json::Int64(productId);

    inventoryBody["quantity"] = 5;

    auto inventoryResponse =
        postJson(
            "/inventory/receive",
            inventoryBody);

    ASSERT_NE(inventoryResponse, nullptr);

    ASSERT_EQ(
        inventoryResponse->getStatusCode(),
        drogon::k200OK);


    // -----------------------------------------------------
    // 4. Request 10 units
    // -----------------------------------------------------

    Json::Value orderBody;

    orderBody["customer_name"] =
        "Insufficient Inventory Customer";

    orderBody["warehouse_id"] =
        Json::Int64(warehouseId);

    Json::Value orderItem;

    orderItem["product_id"] =
        Json::Int64(productId);

    orderItem["quantity"] = 10;

    orderBody["items"] =
        Json::Value(Json::arrayValue);

    orderBody["items"].append(orderItem);


    auto orderResponse =
        postJson("/orders", orderBody);


    // -----------------------------------------------------
    // 5. Verify HTTP failure
    // -----------------------------------------------------

    ASSERT_NE(orderResponse, nullptr);

    EXPECT_EQ(
        orderResponse->getStatusCode(),
        drogon::k400BadRequest);

    auto errorJson =
        orderResponse->getJsonObject();

    ASSERT_NE(errorJson, nullptr);

    ASSERT_TRUE(
        errorJson->isMember("error"));

    EXPECT_EQ(
        (*errorJson)["error"].asString(),
        "Insufficient inventory for product " +
            std::to_string(productId));


    // -----------------------------------------------------
    // 6. Verify inventory was NOT reserved
    // -----------------------------------------------------

    auto inventoryCheckResponse =
        get(
            "/inventory/" +
            std::to_string(warehouseId));

    ASSERT_NE(
        inventoryCheckResponse,
        nullptr);

    ASSERT_EQ(
        inventoryCheckResponse->getStatusCode(),
        drogon::k200OK);

    auto inventoryList =
        inventoryCheckResponse->getJsonObject();

    ASSERT_NE(inventoryList, nullptr);

    ASSERT_TRUE(
        inventoryList->isArray());

    ASSERT_EQ(
        inventoryList->size(),
        1);

    EXPECT_EQ(
        (*inventoryList)[0]["quantity"].asInt(),
        5);

    EXPECT_EQ(
        (*inventoryList)[0]["reserved_quantity"].asInt(),
        0);

    EXPECT_EQ(
        (*inventoryList)[0]["available_quantity"].asInt(),
        5);
}
