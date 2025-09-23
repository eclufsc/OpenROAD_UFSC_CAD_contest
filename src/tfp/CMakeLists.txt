cmake_minimum_required(VERSION 3.14)

project(tfp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Inclua diretórios necessários
include_directories(
    include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/odb/include
    ${CMAKE_SOURCE_DIR}/src/utl/include
    ${CMAKE_SOURCE_DIR}/include
)

# Adicionar src como subdirectory
add_subdirectory(src)
