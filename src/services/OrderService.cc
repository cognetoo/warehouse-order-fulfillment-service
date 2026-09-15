#include "OrderService.h"
#include "../repositories/OrderRepository.h"

#include <unordered_set>

void OrderService::createOrder(
    const Order& order,
    std::function<void(long long)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    // ------------------------------------------------------------
    // Basic validation
    // ------------------------------------------------------------

    if (order.customerName.empty())
    {
        onError("Customer name is required.");
        return;
    }

    if (order.warehouseId <= 0)
    {
        onError("Valid warehouse_id is required.");
        return;
    }

    if (order.items.empty())
    {
        onError("Order must contain at least one item.");
        return;
    }

    // ------------------------------------------------------------
    // Validate individual items
    // ------------------------------------------------------------

    std::unordered_set<long long> productIds;

    for (const auto& item : order.items)
    {
        if (item.productId <= 0)
        {
            onError("Invalid product_id.");
            return;
        }

        if (item.quantity <= 0)
        {
            onError("Item quantity must be greater than zero.");
            return;
        }

        // Don't allow the same product to appear twice
        // in one order.
        if (!productIds.insert(item.productId).second)
        {
            onError(
                "Duplicate product in order: " +
                std::to_string(item.productId));

            return;
        }
    }

    // ------------------------------------------------------------
    // Repository performs the complete transactional workflow.
    // ------------------------------------------------------------

    OrderRepository::createOrderWithReservation(
        order,
        onSuccess,
        onError);
}
