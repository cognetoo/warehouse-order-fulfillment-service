#include <gtest/gtest.h>

#include "../src/services/OrderService.h"

TEST(OrderServiceTest, RejectsEmptyCustomerName)
{
    Order order;

    order.customerName = "";
    order.warehouseId = 1;

    order.items.push_back({1, 5});

    bool errorCalled = false;
    std::string errorMessage;

    OrderService::createOrder(
        order,
        [](long long) {},
        [&](const std::string& error)
        {
            errorCalled = true;
            errorMessage = error;
        });

    EXPECT_TRUE(errorCalled);
    EXPECT_EQ(errorMessage, "Customer name is required.");
}


TEST(OrderServiceTest, RejectsInvalidWarehouse)
{
    Order order;

    order.customerName = "Tarun";
    order.warehouseId = 0;

    order.items.push_back({1, 5});

    bool errorCalled = false;
    std::string errorMessage;

    OrderService::createOrder(
        order,
        [](long long) {},
        [&](const std::string& error)
        {
            errorCalled = true;
            errorMessage = error;
        });

    EXPECT_TRUE(errorCalled);
    EXPECT_EQ(errorMessage, "Valid warehouse_id is required.");
}


TEST(OrderServiceTest, RejectsEmptyItems)
{
    Order order;

    order.customerName = "Tarun";
    order.warehouseId = 1;

    bool errorCalled = false;
    std::string errorMessage;

    OrderService::createOrder(
        order,
        [](long long) {},
        [&](const std::string& error)
        {
            errorCalled = true;
            errorMessage = error;
        });

    EXPECT_TRUE(errorCalled);
    EXPECT_EQ(errorMessage, "Order must contain at least one item.");
}


TEST(OrderServiceTest, RejectsInvalidProductId)
{
    Order order;

    order.customerName = "Tarun";
    order.warehouseId = 1;

    order.items.push_back({0, 5});

    bool errorCalled = false;
    std::string errorMessage;

    OrderService::createOrder(
        order,
        [](long long) {},
        [&](const std::string& error)
        {
            errorCalled = true;
            errorMessage = error;
        });

    EXPECT_TRUE(errorCalled);
    EXPECT_EQ(errorMessage, "Invalid product_id.");
}


TEST(OrderServiceTest, RejectsInvalidQuantity)
{
    Order order;

    order.customerName = "Tarun";
    order.warehouseId = 1;

    order.items.push_back({1, 0});

    bool errorCalled = false;
    std::string errorMessage;

    OrderService::createOrder(
        order,
        [](long long) {},
        [&](const std::string& error)
        {
            errorCalled = true;
            errorMessage = error;
        });

    EXPECT_TRUE(errorCalled);
    EXPECT_EQ(errorMessage,
              "Item quantity must be greater than zero.");
}


TEST(OrderServiceTest, RejectsDuplicateProduct)
{
    Order order;

    order.customerName = "Tarun";
    order.warehouseId = 1;

    order.items.push_back({1, 5});
    order.items.push_back({1, 10});

    bool errorCalled = false;
    std::string errorMessage;

    OrderService::createOrder(
        order,
        [](long long) {},
        [&](const std::string& error)
        {
            errorCalled = true;
            errorMessage = error;
        });

    EXPECT_TRUE(errorCalled);
    EXPECT_EQ(
        errorMessage,
        "Duplicate product in order: 1");
}


