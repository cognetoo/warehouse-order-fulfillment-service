ALTER TABLE orders
ADD COLUMN warehouse_id BIGINT;

UPDATE orders
SET warehouse_id = (
    SELECT id
    FROM warehouses
    ORDER BY id
    LIMIT 1
)
WHERE warehouse_id IS NULL;

ALTER TABLE orders
ALTER COLUMN warehouse_id SET NOT NULL;

ALTER TABLE orders
ADD CONSTRAINT fk_orders_warehouse
FOREIGN KEY (warehouse_id)
REFERENCES warehouses(id);
