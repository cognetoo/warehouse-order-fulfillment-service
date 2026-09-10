CREATE TABLE inventory (
    warehouse_id BIGINT NOT NULL,

    product_id BIGINT NOT NULL,

    quantity INTEGER NOT NULL DEFAULT 0,

    reserved_quantity INTEGER NOT NULL DEFAULT 0,

    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (warehouse_id, product_id),

    FOREIGN KEY (warehouse_id)
        REFERENCES warehouses(id),

    FOREIGN KEY (product_id)
        REFERENCES products(id),

    CHECK (quantity >= 0),

    CHECK (reserved_quantity >= 0),

    CHECK (reserved_quantity <= quantity)
);
