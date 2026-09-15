#include "OrderRepository.h"

#include <drogon/drogon.h>

namespace
{

using TransactionPtr = std::shared_ptr<drogon::orm::Transaction>;

struct TransactionState
{
    long long orderId = -1;
    std::string failureReason;
};


// ------------------------------------------------------------
// Rollback helper
// ------------------------------------------------------------

void rollbackWithError(
    const TransactionPtr& transaction,
    const std::shared_ptr<TransactionState>& state,
    const std::string& message,
    const std::function<void(const std::string&)>& onError)
{
    state->failureReason = message;

    transaction->rollback();

    // A manual rollback does not trigger the commit callback.
    // Therefore, explicitly notify the HTTP layer of the error.
    onError(state->failureReason);
}


// ------------------------------------------------------------
// Process order items one by one
// ------------------------------------------------------------

void processOrderItems(
    const TransactionPtr& transaction,
    const Order& order,
    const std::shared_ptr<TransactionState>& state,
    size_t index,
    const std::function<void(const std::string&)>& onError)
{
    // ------------------------------------------------------------
    // All items processed
    // ------------------------------------------------------------

    if (index >= order.items.size())
    {
        transaction->execSqlAsync(
            "UPDATE orders "
            "SET status = 'CONFIRMED' "
            "WHERE id = $1",

            [](const drogon::orm::Result&)
            {
                // Nothing to do here.
                //
                // If this succeeds, the transaction will be
                // committed when the transaction object is released.
            },

            [transaction, state, onError](
                const drogon::orm::DrogonDbException& e)
            {
                rollbackWithError(
                    transaction,
                    state,
                    std::string("Failed to confirm order: ") +
                        e.base().what(),
                    onError);
            },

            state->orderId);

        return;
    }


    const auto& item = order.items[index];


    // ------------------------------------------------------------
    // Insert order item
    // ------------------------------------------------------------

    transaction->execSqlAsync(
        "INSERT INTO order_items "
        "(order_id, product_id, quantity) "
        "VALUES ($1, $2, $3)",

        [transaction, order, state, index, onError](
            const drogon::orm::Result&)
        {
            const auto& item = order.items[index];


            // ----------------------------------------------------
            // Reserve inventory
            // ----------------------------------------------------

            transaction->execSqlAsync(
                "UPDATE inventory "
                "SET reserved_quantity = reserved_quantity + $1, "
                "    updated_at = CURRENT_TIMESTAMP "
                "WHERE warehouse_id = $2 "
                "  AND product_id = $3 "
                "  AND quantity - reserved_quantity >= $1 "
                "RETURNING quantity, reserved_quantity",

                [transaction, order, state, index, onError](
                    const drogon::orm::Result& result)
                {
                    const auto& item = order.items[index];


                    // No row means there wasn't enough
                    // available inventory.
                    if (result.empty())
                    {
                        rollbackWithError(
                            transaction,
                            state,
                            "Insufficient inventory for product " +
                                std::to_string(item.productId),
                            onError);

                        return;
                    }


                    // ------------------------------------------------
                    // Reservation succeeded.
                    // Process the next item.
                    // ------------------------------------------------

                    processOrderItems(
                        transaction,
                        order,
                        state,
                        index + 1,
                        onError);
                },

                [transaction, state, onError](
                    const drogon::orm::DrogonDbException& e)
                {
                    rollbackWithError(
                        transaction,
                        state,
                        std::string("Failed to reserve inventory: ") +
                            e.base().what(),
                        onError);
                },

                item.quantity,
                order.warehouseId,
                item.productId);
        },

        [transaction, state, onError](
            const drogon::orm::DrogonDbException& e)
        {
            rollbackWithError(
                transaction,
                state,
                std::string("Failed to create order item: ") +
                    e.base().what(),
                onError);
        },

        state->orderId,
        item.productId,
        item.quantity);
}

} // namespace


// ============================================================
// CREATE ORDER + RESERVE INVENTORY
// ============================================================

void OrderRepository::createOrderWithReservation(
    const Order& order,
    std::function<void(long long)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient =
        drogon::app().getDbClient("default");


    // ------------------------------------------------------------
    // START TRANSACTION
    // ------------------------------------------------------------

    dbClient->newTransactionAsync(
        [order, onSuccess, onError](
            const TransactionPtr& transaction)
        {
            if (!transaction)
            {
                onError(
                    "Failed to start database transaction.");

                return;
            }


            auto state =
                std::make_shared<TransactionState>();


            // ----------------------------------------------------
            // Commit callback
            //
            // This is only responsible for successful commits.
            // Rollback errors are handled explicitly through
            // rollbackWithError().
            // ----------------------------------------------------

            transaction->setCommitCallback(
                [state, onSuccess](
                    bool committed)
                {
                    if (committed)
                    {
                        onSuccess(state->orderId);
                    }
                });


            // ----------------------------------------------------
            // STEP 1
            // Create main order
            // ----------------------------------------------------

            transaction->execSqlAsync(
                "INSERT INTO orders "
                "(customer_name, warehouse_id, status) "
                "VALUES ($1, $2, 'PENDING') "
                "RETURNING id",

                [transaction, order, state, onError](
                    const drogon::orm::Result& result)
                {
                    if (result.empty())
                    {
                        rollbackWithError(
                            transaction,
                            state,
                            "Failed to create order.",
                            onError);

                        return;
                    }


                    // Save generated order ID.
                    state->orderId =
                        result[0]["id"].as<long long>();


                    // ------------------------------------------------
                    // STEP 2
                    // Process order items
                    // ------------------------------------------------

                    processOrderItems(
                        transaction,
                        order,
                        state,
                        0,
                        onError);
                },

                [transaction, state, onError](
                    const drogon::orm::DrogonDbException& e)
                {
                    rollbackWithError(
                        transaction,
                        state,
                        std::string("Failed to create order: ") +
                            e.base().what(),
                        onError);
                },

                order.customerName,
                order.warehouseId);
});
}


// ============================================================
// GET ORDER
// ============================================================

void OrderRepository::getOrder(
    long long orderId,
    std::function<void(const Order&)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient =
        drogon::app().getDbClient("default");


    dbClient->execSqlAsync(
        "SELECT "
        "o.id, "
        "o.warehouse_id, "
        "o.customer_name, "
        "o.status, "
        "oi.product_id, "
        "oi.quantity "
        "FROM orders o "
        "LEFT JOIN order_items oi "
        "ON o.id = oi.order_id "
        "WHERE o.id = $1 "
        "ORDER BY oi.product_id",

        [onSuccess, onError](
            const drogon::orm::Result& result)
        {
            if (result.empty())
            {
                onError("Order not found.");
                return;
            }


            Order order;


            order.id =
                result[0]["id"].as<long long>();


            order.warehouseId =
                result[0]["warehouse_id"]
                    .as<long long>();


            order.customerName =
                result[0]["customer_name"]
                    .as<std::string>();


            order.status =
                result[0]["status"]
                    .as<std::string>();


            // ----------------------------------------------------
            // Build order items
            // ----------------------------------------------------

            for (const auto& row : result)
            {
                if (!row["product_id"].isNull())
                {
                    OrderItem item;


                    item.productId =
                        row["product_id"]
                            .as<long long>();


                    item.quantity =
                        row["quantity"]
                            .as<int>();


                    order.items.push_back(item);
                }
            }


            onSuccess(order);
        },

        [onError](
            const drogon::orm::DrogonDbException& e)
        {
            onError(
                std::string("Failed to fetch order: ") +
                e.base().what());
        },

        orderId);
}
