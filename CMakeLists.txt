cmake_minimum_required(VERSION 3.20)

project(MahadevRiceMillERP VERSION 2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Disable private module warning for internal qmltc vtables
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# Find Qt 6 Packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
)

# Find private/template/layout packages for cross-platform qmltc compilation (Windows, macOS, Linux)
find_package(Qt6 OPTIONAL_COMPONENTS
    CorePrivate
    QmlPrivate
    QuickPrivate
    QuickTemplates2Private
    QuickLayoutsPrivate
    QuickDialogs2Private
    QmlMetaPrivate
)

# Embedded SQLite Amalgamation
add_library(sqlite3_embedded STATIC
    third_party/sqlite/sqlite3.c
    third_party/sqlite/sqlite3.h
)
set_target_properties(sqlite3_embedded PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)
target_include_directories(sqlite3_embedded PUBLIC third_party/sqlite)
target_compile_definitions(sqlite3_embedded PRIVATE
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_RTREE
    SQLITE_ENABLE_JSON1
    SQLITE_THREADSAFE=1
    SQLITE_DEFAULT_WAL_SYNCHRONOUS=1
)

# Embedded MDB (Access Jet3/Jet4) Engine - 100% In-Tree Standalone C
file(GLOB LIBMDB_SOURCES "third_party/libmdb/*.c")
add_library(libmdb_embedded STATIC ${LIBMDB_SOURCES})
set_target_properties(libmdb_embedded PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)
target_include_directories(libmdb_embedded PUBLIC third_party/libmdb)
target_compile_definitions(libmdb_embedded PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
)
if (MSVC)
    target_compile_options(libmdb_embedded PRIVATE $<$<COMPILE_LANGUAGE:C>:/std:c11>)
endif()

# Application Sources
set(SOURCES
    src/main.cpp
    src/database_manager.h
    src/database_manager.cpp
    src/engine/accounting_engine.h
    src/models/base_table_model.h
    src/models/base_table_model.cpp
    src/models/dashboard_controller.h
    src/models/dashboard_controller.cpp
    src/models/paddy_arrivals_model.h
    src/models/paddy_arrivals_model.cpp
    src/models/account_groups_model.h
    src/models/account_groups_model.cpp
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
    src/models/firm_manager.h
    src/models/firm_manager.cpp
    src/models/jform_model.h
    src/models/jform_model.cpp
    src/models/tds_model.h
    src/models/tds_model.cpp
    src/models/generic_list_model.h
    src/models/generic_list_model.cpp
    src/engine/bahi_khata_migrator.h
    src/engine/bahi_khata_migrator.cpp
)

qt_add_executable(${PROJECT_NAME} ${SOURCES})

# Strict QML-to-C++ AOT Type Compilation via qmltc (qmlcachegen disabled to eliminate bytecode caching overhead)
qt_add_qml_module(${PROJECT_NAME}
    URI MahadevERP
    VERSION 1.0
    RESOURCE_PREFIX /
    ENABLE_TYPE_COMPILER
    NO_CACHEGEN
    DEPENDENCIES
        QtQuick
        QtQuick.Layouts
        QtQuick.Templates
    SOURCES
        src/models/generic_list_model.h
        src/models/generic_list_model.cpp
    QML_FILES
        qml/main.qml
        qml/components/AppHeader.qml
        qml/components/AutoCompletePartySearch.qml
        qml/components/CustomInput.qml
        qml/components/CustomWhiteCombo.qml
        qml/components/CustomCheckBox.qml
        qml/components/CustomRadioButton.qml
        qml/components/FastTable.qml
        qml/components/FormNavigationScope.qml
        qml/components/KbdBadge.qml
        qml/components/NavMenuItem.qml
        qml/components/Sidebar.qml
        qml/components/StatCard.qml
        qml/dialogs/AccountingPeriodModal.qml
        qml/dialogs/AddVoucherMenuModal.qml
        qml/dialogs/ConfirmationModal.qml
        qml/dialogs/ItemMovementModal.qml
        qml/dialogs/JFormStubModal.qml
        qml/dialogs/KeyboardShortcutsModal.qml
        qml/dialogs/LedgerMasterMenuModal.qml
        qml/dialogs/MdbMigrationModal.qml
        qml/dialogs/NewLedgerModal.qml
        qml/dialogs/NewPaddyModal.qml
        qml/dialogs/OtherVouchersMenuModal.qml
        qml/dialogs/ReportsMenuModal.qml
        qml/dialogs/StockMasterMenuModal.qml
        qml/dialogs/NewFirmModal.qml
        qml/views/ChequeVoucherView.qml
        qml/views/DashboardView.qml
        qml/views/FirmSelecterView.qml
        qml/views/JFormVoucherView.qml
        qml/views/JournalVoucherView.qml
        qml/views/MillingStatementView.qml
        qml/views/MillingView.qml
        qml/views/MillingVoucherView.qml
        qml/views/ModifyGroupView.qml
        qml/views/ModifyLedgerView.qml
        qml/views/ModifyStockItemView.qml
        qml/views/NewGroupView.qml
        qml/views/NewLedgerView.qml
        qml/views/NewStockItemView.qml
        qml/views/PaddyProcurementView.qml
        qml/views/PurchaseRegisterView.qml
        qml/views/PurchaseVoucherView.qml
        qml/views/ReportsView.qml
        qml/views/SalesInvoicingView.qml
        qml/views/SalesRegisterView.qml
        qml/views/SalesVoucherView.qml
        qml/views/StockDetailView.qml
        qml/views/ViewLedgerStatementView.qml
        qml/views/VoucherLedgerView.qml
        qml/views/TdsVoucherView.qml
)

# Cross-Platform Qt Private Header Include Resolution
if(WIN32)
    # Windows layout (MSVC / MinGW): <QT_DIR>/include/<Module>/<Version>/<Module>
    file(GLOB WIN_QT_PRIV_DIRS
        LIST_DIRECTORIES true
        "${QT6_INSTALL_PREFIX}/include/*/${Qt6Core_VERSION}"
        "${QT6_INSTALL_PREFIX}/include/*/${Qt6Core_VERSION}/*"
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${WIN_QT_PRIV_DIRS})
elseif(APPLE)
    # macOS layout: Homebrew Framework / official SDK layout
    file(GLOB MAC_QT_PRIV_DIRS
        LIST_DIRECTORIES true
        "/opt/homebrew/lib/Qt*.framework/Headers/${Qt6Core_VERSION}"
        "/opt/homebrew/lib/Qt*.framework/Headers/${Qt6Core_VERSION}/*"
        "/opt/homebrew/Cellar/*/*/lib/Qt*.framework/Versions/A/Headers/${Qt6Core_VERSION}"
        "/opt/homebrew/Cellar/*/*/lib/Qt*.framework/Versions/A/Headers/${Qt6Core_VERSION}/*"
        "${QT6_INSTALL_PREFIX}/lib/Qt*.framework/Headers/${Qt6Core_VERSION}"
        "${QT6_INSTALL_PREFIX}/lib/Qt*.framework/Headers/${Qt6Core_VERSION}/*"
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${MAC_QT_PRIV_DIRS})
else()
    # Linux layout: Distro / Online installer
    file(GLOB LINUX_QT_PRIV_DIRS
        LIST_DIRECTORIES true
        "${QT6_INSTALL_PREFIX}/include/*/${Qt6Core_VERSION}"
        "${QT6_INSTALL_PREFIX}/include/*/${Qt6Core_VERSION}/*"
        "/usr/include/qt6/*/${Qt6Core_VERSION}"
        "/usr/include/qt6/*/${Qt6Core_VERSION}/*"
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${LINUX_QT_PRIV_DIRS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/engine
    src/models
    third_party/sqlite
    third_party/libmdb
)

target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_LIBMDB=1)

target_link_libraries(${PROJECT_NAME} PRIVATE
    sqlite3_embedded
    libmdb_embedded
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    $<$<TARGET_EXISTS:Qt6::CorePrivate>:Qt6::CorePrivate>
    $<$<TARGET_EXISTS:Qt6::QmlPrivate>:Qt6::QmlPrivate>
    $<$<TARGET_EXISTS:Qt6::QuickPrivate>:Qt6::QuickPrivate>
    $<$<TARGET_EXISTS:Qt6::QuickTemplates2Private>:Qt6::QuickTemplates2Private>
    $<$<TARGET_EXISTS:Qt6::QuickLayoutsPrivate>:Qt6::QuickLayoutsPrivate>
    $<$<TARGET_EXISTS:Qt6::QuickDialogs2Private>:Qt6::QuickDialogs2Private>
    $<$<TARGET_EXISTS:Qt6::QmlMetaPrivate>:Qt6::QmlMetaPrivate>
)

# On macOS Framework installations, ensure required framework binaries link directly
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework QtQuickTemplates2"
        "-framework QtQuickLayouts"
        "-framework QtQuickDialogs2"
        "-framework QtQmlMeta"
    )
endif()

# Set macOS App Bundle Properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Mahadev Rice Mill ERP"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mahadevagro.ricemill"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "2.0"
    MACOSX_BUNDLE_VERSION "2.0"
)
