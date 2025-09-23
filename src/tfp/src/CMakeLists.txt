set(TFP_SOURCES
    TechFileParser.cpp
    TechFileCommands.cpp
)

add_library(tfp ${TFP_SOURCES})

target_include_directories(tfp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_SOURCE_DIR}/src/odb/include
        ${CMAKE_SOURCE_DIR}/src/utl/include
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tfp
    PUBLIC
        odb
        utl
)
