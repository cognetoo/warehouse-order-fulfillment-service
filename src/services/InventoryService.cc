#include "InventoryService.h"

#include "../repositories/InventoryRepository.h"

void InventoryService::receiveInventory(
    const Inventory& inventory,
    std::function<void()> onSuccess,
    std::function<void(const std::string&)> onError)
{
    if (inventory.warehouseId <= 0)
    {
        onError("Invalid warehouse ID");
        return;
    }

    if (inventory.productId <= 0)
    {
        onError("Invalid product ID");
        return;
    }

    if (inventory.quantity <= 0)
    {
        onError("Quantity must be greater than zero");
        return;
    }

    InventoryRepository::receiveInventory(
        inventory,
        onSuccess,
        onError
    );
}
