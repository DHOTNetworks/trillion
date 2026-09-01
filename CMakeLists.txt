cmake_minimum_required(VERSION 3.20)

project(MahadevRiceMillERP VERSION 2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Option for standalone static build
option(BUILD_STATIC "Build standalone static binary with zero runtime dependencies" OFF)

if(BUILD_STATIC)
    message(STATUS "Configuring for Standalone Static Binary Build...")
    if(MSVC)
        # Statically link MSVC C/C++ Runtime (/MT)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# Find Qt 6 Packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# Embedded SQLite Amalgamation
add_library(sqlite3_embedded STATIC
    third_party/sqlite/sqlite3.c
    third_party/sqlite/sqlite3.h
)
target_include_directories(sqlite3_embedded PUBLIC third_party/sqlite)
target_compile_definitions(sqlite3_embedded PRIVATE
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_RTREE
    SQLITE_ENABLE_JSON1
    SQLITE_THREADSAFE=1
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
)

# Application Sources
set(SOURCES
    src/main.cpp
    src/database_manager.h
    src/database_manager.cpp
    src/engine/accounting_engine.h
    src/models/base_table_model.h
    src/models/base_table_model.cpp
    src/models/sales_model.h
    src/models/sales_model.cpp
    src/models/purchase_model.h
    src/models/purchase_model.cpp
    src/models/vouchers_model.h
    src/models/vouchers_model.cpp
    src/models/stock_items_model.h
    src/models/stock_items_model.cpp
    src/models/parties_model.h
    src/models/parties_model.cpp
    src/models/milling_model.h
    src/models/milling_model.cpp
    src/models/financial_years_model.h
    src/models/financial_years_model.cpp
    qml.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/engine
    src/models
    third_party/sqlite
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    sqlite3_embedded
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Mahadev ERP"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mahadev.erp"
    )
elseif(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

message(STATUS "Mahadev Rice Mill ERP C++ Project configured successfully!")
