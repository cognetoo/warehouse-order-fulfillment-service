#include "ProductRepository.h"

void ProductRepository::createProduct(
    const Product& product,
    std::function<void(long long)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient = drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "INSERT INTO products (sku, name, description, price) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING id",

        [onSuccess](const drogon::orm::Result& result)
        {
            long long id =
                result[0]["id"].as<long long>();

            onSuccess(id);
        },

        [onError](const drogon::orm::DrogonDbException& e)
        {
            onError(e.base().what());
        },

        product.sku,
        product.name,
        product.description,
        product.price
    );
}
void ProductRepository::getAllProducts(
    std::function<void(const drogon::orm::Result&)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient =
        drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT id, sku, name, description, price "
        "FROM products "
        "ORDER BY id",

        [onSuccess](const drogon::orm::Result& result)
        {
            onSuccess(result);
        },

        [onError](const drogon::orm::DrogonDbException& e)
        {
            onError(e.base().what());
        }
    );
}
void ProductRepository::getProductById(
    long long id,
    std::function<void(const drogon::orm::Result&)> onSuccess,
    std::function<void(const std::string&)> onError)
{
    auto dbClient =
        drogon::app().getDbClient("default");

    dbClient->execSqlAsync(
        "SELECT id, sku, name, description, price "
        "FROM products "
        "WHERE id = $1",

        [onSuccess](const drogon::orm::Result& result)
        {
            onSuccess(result);
        },

        [onError](const drogon::orm::DrogonDbException& e)
        {
            onError(e.base().what());
        },

        id
    );
}
