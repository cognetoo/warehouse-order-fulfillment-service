cmake_minimum_required(VERSION 3.16)

project(warehouse_service)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

add_executable(warehouse_service
    src/main.cc
    src/controllers/ProductController.cc
    src/repositories/ProductRepository.cc
    src/controllers/WarehouseController.cc
    src/repositories/WarehouseRepository.cc
)

target_link_libraries(warehouse_service
    PRIVATE Drogon::Drogon
)
