#include <gtest/gtest.h>
#include <drogon/drogon.h>

#include "../src/repositories/OrderRepository.h"

#include <future>
#include <string>
#include <thread>
#include <chrono>

class OrderRepositoryTest : public ::testing::Test
{
protected:
    static std::thread appThread;

    static void SetUpTestSuite()
    {
        drogon::app().loadConfigFile(
            "tests/config/config.json");

        appThread = std::thread([]()
        {
            drogon::app().run();
        });

        // Give Drogon time to initialize its database clients.
        for (int i = 0; i < 50; ++i)
        {
            if (drogon::app().hasDbClient("default"))
            {
                return;
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
        }

        throw std::runtime_error(
            "Drogon database client failed to initialize.");
    }

    static void TearDownTestSuite()
    {
        drogon::app().quit();

        if (appThread.joinable())
        {
            appThread.join();
        }
    }
};

std::thread OrderRepositoryTest::appThread;

TEST_F(OrderRepositoryTest, CreatesOrderAndReservesInventory)
{
    auto dbClient =
        drogon::app().getDbClient("default");

    // Clean test data.
    dbClient->execSqlSync(
        "DELETE FROM order_items");

    dbClient->execSqlSync(
        "DELETE FROM orders");

    dbClient->execSqlSync(
        "DELETE FROM inventory");

    dbClient->execSqlSync(
        "DELETE FROM products");

    dbClient->execSqlSync(
        "DELETE FROM warehouses");

    // Create warehouse.
    auto warehouseResult =
        dbClient->execSqlSync(
            "INSERT INTO warehouses "
            "(name, location) "
            "VALUES ('Test Warehouse', 'Bangalore') "
            "RETURNING id");

    long long warehouseId =
        warehouseResult[0]["id"].as<long long>();

    // Create product.
    auto productResult =
        dbClient->execSqlSync(
            "INSERT INTO products "
            "(sku, name, description, price) "
            "VALUES "
            "('TEST-001', 'Test Product', 'Integration Test', 1000) "
            "RETURNING id");

    long long productId =
        productResult[0]["id"].as<long long>();

    // Add inventory.
    dbClient->execSqlSync(
        "INSERT INTO inventory "
        "(warehouse_id, product_id, quantity, reserved_quantity) "
        "VALUES ($1, $2, 50, 0)",
        warehouseId,
        productId);

    // Create order.
    Order order;

    order.customerName = "Integration Test Customer";
    order.warehouseId = warehouseId;

    order.items.push_back({
        productId,
        10
    });

    std::promise<long long> successPromise;
    std::promise<std::string> errorPromise;

    auto successFuture =
        successPromise.get_future();

    auto errorFuture =
        errorPromise.get_future();

    OrderRepository::createOrderWithReservation(
        order,
        [&successPromise](long long orderId)
        {
            successPromise.set_value(orderId);
        },
        [&errorPromise](const std::string& error)
        {
            errorPromise.set_value(error);
        });

    auto status =
        successFuture.wait_for(
            std::chrono::seconds(5));

    ASSERT_EQ(
        status,
        std::future_status::ready);

    long long orderId =
        successFuture.get();

    // Verify order exists.
    auto orderResult =
        dbClient->execSqlSync(
            "SELECT status, customer_name, warehouse_id "
            "FROM orders "
            "WHERE id = $1",
            orderId);

    ASSERT_EQ(orderResult.size(), 1);

    EXPECT_EQ(
        orderResult[0]["status"].as<std::string>(),
        "CONFIRMED");

    EXPECT_EQ(
        orderResult[0]["customer_name"].as<std::string>(),
        "Integration Test Customer");

    EXPECT_EQ(
        orderResult[0]["warehouse_id"].as<long long>(),
        warehouseId);

    // Verify order item exists.
    auto itemResult =
        dbClient->execSqlSync(
            "SELECT quantity "
            "FROM order_items "
            "WHERE order_id = $1 "
            "AND product_id = $2",
            orderId,
            productId);

    ASSERT_EQ(itemResult.size(), 1);

    EXPECT_EQ(
        itemResult[0]["quantity"].as<int>(),
        10);

    // Verify inventory reservation.
    auto inventoryResult =
        dbClient->execSqlSync(
            "SELECT quantity, reserved_quantity "
            "FROM inventory "
            "WHERE warehouse_id = $1 "
            "AND product_id = $2",
            warehouseId,
            productId);

    ASSERT_EQ(inventoryResult.size(), 1);

    EXPECT_EQ(
        inventoryResult[0]["quantity"].as<int>(),
        50);

    EXPECT_EQ(
        inventoryResult[0]["reserved_quantity"].as<int>(),
        10);
}
TEST_F(OrderRepositoryTest, RollsBackWhenInventoryIsInsufficient)
{
    auto dbClient =
        drogon::app().getDbClient("default");

    // Clean test data.
    dbClient->execSqlSync(
        "DELETE FROM order_items");

    dbClient->execSqlSync(
        "DELETE FROM orders");

    dbClient->execSqlSync(
        "DELETE FROM inventory");

    dbClient->execSqlSync(
        "DELETE FROM products");

    dbClient->execSqlSync(
        "DELETE FROM warehouses");

    // Create warehouse.
    auto warehouseResult =
        dbClient->execSqlSync(
            "INSERT INTO warehouses "
            "(name, location) "
            "VALUES ('Rollback Test Warehouse', 'Bangalore') "
            "RETURNING id");

    long long warehouseId =
        warehouseResult[0]["id"].as<long long>();

    // Create product.
    auto productResult =
        dbClient->execSqlSync(
            "INSERT INTO products "
            "(sku, name, description, price) "
            "VALUES "
            "('ROLLBACK-001', 'Rollback Test Product', "
            "'Integration Test', 1000) "
            "RETURNING id");

    long long productId =
        productResult[0]["id"].as<long long>();

    // Only 5 units available.
    dbClient->execSqlSync(
        "INSERT INTO inventory "
        "(warehouse_id, product_id, quantity, reserved_quantity) "
        "VALUES ($1, $2, 5, 0)",
        warehouseId,
        productId);

    // Request 10 units — more than available.
    Order order;

    order.customerName = "Rollback Test Customer";
    order.warehouseId = warehouseId;

    order.items.push_back({
        productId,
        10
    });

    std::promise<long long> successPromise;
    std::promise<std::string> errorPromise;

    auto successFuture =
        successPromise.get_future();

    auto errorFuture =
        errorPromise.get_future();

    OrderRepository::createOrderWithReservation(
        order,
        [&successPromise](long long orderId)
        {
            successPromise.set_value(orderId);
        },
        [&errorPromise](const std::string& error)
        {
            errorPromise.set_value(error);
        });

    // We expect the transaction to fail.
    auto status =
        errorFuture.wait_for(
            std::chrono::seconds(5));

    ASSERT_EQ(
        status,
        std::future_status::ready);

    std::string error =
        errorFuture.get();

    EXPECT_EQ(
        error,
        "Insufficient inventory for product " +
        std::to_string(productId));

    // Verify that NO order was committed.
    auto orderResult =
        dbClient->execSqlSync(
            "SELECT COUNT(*) AS count "
            "FROM orders "
            "WHERE customer_name = $1",
            "Rollback Test Customer");

    ASSERT_EQ(orderResult.size(), 1);

    EXPECT_EQ(
        orderResult[0]["count"].as<long long>(),
        0);

    // Verify that NO order item was committed.
    auto itemResult =
        dbClient->execSqlSync(
            "SELECT COUNT(*) AS count "
            "FROM order_items oi "
            "JOIN orders o ON o.id = oi.order_id "
            "WHERE o.customer_name = $1",
            "Rollback Test Customer");

    ASSERT_EQ(itemResult.size(), 1);

    EXPECT_EQ(
        itemResult[0]["count"].as<long long>(),
        0);

    // Verify inventory was not modified.
    auto inventoryResult =
        dbClient->execSqlSync(
            "SELECT quantity, reserved_quantity "
            "FROM inventory "
            "WHERE warehouse_id = $1 "
            "AND product_id = $2",
            warehouseId,
            productId);

    ASSERT_EQ(inventoryResult.size(), 1);

    EXPECT_EQ(
        inventoryResult[0]["quantity"].as<int>(),
        5);

    EXPECT_EQ(
        inventoryResult[0]["reserved_quantity"].as<int>(),
        0);
}
TEST_F(OrderRepositoryTest, HandlesConcurrentReservations)
{
    auto dbClient =
        drogon::app().getDbClient("default");

    // Clean test data.
    dbClient->execSqlSync(
        "DELETE FROM order_items");

    dbClient->execSqlSync(
        "DELETE FROM orders");

    dbClient->execSqlSync(
        "DELETE FROM inventory");

    dbClient->execSqlSync(
        "DELETE FROM products");

    dbClient->execSqlSync(
        "DELETE FROM warehouses");

    // Create warehouse.
    auto warehouseResult =
        dbClient->execSqlSync(
            "INSERT INTO warehouses "
            "(name, location) "
            "VALUES ('Concurrency Test Warehouse', 'Bangalore') "
            "RETURNING id");

    long long warehouseId =
        warehouseResult[0]["id"].as<long long>();

    // Create product.
    auto productResult =
        dbClient->execSqlSync(
            "INSERT INTO products "
            "(sku, name, description, price) "
            "VALUES "
            "('CONCURRENT-001', 'Concurrency Test Product', "
            "'Concurrency Integration Test', 1000) "
            "RETURNING id");

    long long productId =
        productResult[0]["id"].as<long long>();

    // Only 20 units available.
    dbClient->execSqlSync(
        "INSERT INTO inventory "
        "(warehouse_id, product_id, quantity, reserved_quantity) "
        "VALUES ($1, $2, 20, 0)",
        warehouseId,
        productId);

    Order orderA;
    orderA.customerName = "Concurrent Customer A";
    orderA.warehouseId = warehouseId;
    orderA.items.push_back({productId, 15});

    Order orderB;
    orderB.customerName = "Concurrent Customer B";
    orderB.warehouseId = warehouseId;
    orderB.items.push_back({productId, 15});

    std::promise<std::string> resultPromiseA;
    std::promise<std::string> resultPromiseB;

    auto resultFutureA =
        resultPromiseA.get_future();

    auto resultFutureB =
        resultPromiseB.get_future();

    OrderRepository::createOrderWithReservation(
        orderA,
        [&resultPromiseA](long long)
        {
            resultPromiseA.set_value("SUCCESS");
        },
        [&resultPromiseA](const std::string&)
        {
            resultPromiseA.set_value("FAILURE");
        });

    OrderRepository::createOrderWithReservation(
        orderB,
        [&resultPromiseB](long long)
        {
            resultPromiseB.set_value("SUCCESS");
        },
        [&resultPromiseB](const std::string&)
        {
            resultPromiseB.set_value("FAILURE");
        });

    // Both requests should finish.
    ASSERT_EQ(
        resultFutureA.wait_for(
            std::chrono::seconds(5)),
        std::future_status::ready);

    ASSERT_EQ(
        resultFutureB.wait_for(
            std::chrono::seconds(5)),
        std::future_status::ready);

    std::string resultA =
        resultFutureA.get();

    std::string resultB =
        resultFutureB.get();

    // Exactly one request must succeed.
    EXPECT_NE(resultA, resultB);

    EXPECT_TRUE(
        (resultA == "SUCCESS" && resultB == "FAILURE") ||
        (resultA == "FAILURE" && resultB == "SUCCESS"));

    // Verify final inventory.
    auto inventoryResult =
        dbClient->execSqlSync(
            "SELECT quantity, reserved_quantity "
            "FROM inventory "
            "WHERE warehouse_id = $1 "
            "AND product_id = $2",
            warehouseId,
            productId);

    ASSERT_EQ(inventoryResult.size(), 1);

    EXPECT_EQ(
        inventoryResult[0]["quantity"].as<int>(),
        20);

    EXPECT_EQ(
        inventoryResult[0]["reserved_quantity"].as<int>(),
        15);

    // Exactly one order should have been committed.
    auto orderResult =
        dbClient->execSqlSync(
            "SELECT COUNT(*) AS count "
            "FROM orders "
            "WHERE customer_name IN "
            "('Concurrent Customer A', "
            "'Concurrent Customer B')");

    ASSERT_EQ(orderResult.size(), 1);

    EXPECT_EQ(
        orderResult[0]["count"].as<long long>(),
        1);
}
