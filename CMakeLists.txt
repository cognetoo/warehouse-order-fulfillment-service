cmake_minimum_required(VERSION 3.16)

project(warehouse_service)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

# -----------------------------
# Main application
# -----------------------------

add_executable(warehouse_service
    src/main.cc

    src/controllers/ProductController.cc
    src/repositories/ProductRepository.cc

    src/controllers/WarehouseController.cc
    src/repositories/WarehouseRepository.cc

    src/controllers/InventoryController.cc
    src/repositories/InventoryRepository.cc
    src/services/InventoryService.cc

    src/controllers/OrderController.cc
    src/repositories/OrderRepository.cc
    src/services/OrderService.cc
)

target_link_libraries(warehouse_service
    PRIVATE Drogon::Drogon
)

# -----------------------------
# Testing
# -----------------------------

enable_testing()

find_package(GTest REQUIRED)

add_executable(warehouse_tests
    tests/OrderServiceTest.cc
    tests/OrderRepositoryTest.cc

    src/services/OrderService.cc
    src/repositories/OrderRepository.cc
)

target_link_libraries(warehouse_tests
    PRIVATE
    GTest::gtest_main
    Drogon::Drogon
)

include(GoogleTest)

gtest_discover_tests(warehouse_tests)
