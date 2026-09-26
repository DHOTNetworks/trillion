cmake_minimum_required(VERSION 3.20)

project(MahadevRiceMillERP VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Cross-platform compiler flags (MSVC / Clang / GCC)
if(MSVC)
    add_compile_options(/utf-8 /MP)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif()

# Find Qt 6 Packages
find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Sql
    Svg
    Widgets
    PrintSupport
    Test
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

# Embedded Miniz (ZLIB / Flate Decompression Engine) - 100% In-Tree Standalone C
file(GLOB MINIZ_SOURCES "third_party/miniz/*.c")
add_library(miniz_embedded STATIC ${MINIZ_SOURCES})
set_target_properties(miniz_embedded PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)
target_include_directories(miniz_embedded PUBLIC third_party/miniz)
target_compile_definitions(miniz_embedded PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
)

# Embedded libxls (Excel 97-2004 BIFF8 Binary Engine) - 100% In-Tree Standalone C
set(LIBXLS_SOURCES
    third_party/libxls/src/endian.c
    third_party/libxls/src/locale.c
    third_party/libxls/src/ole.c
    third_party/libxls/src/xls.c
    third_party/libxls/src/xlstool.c
)
add_library(libxls_embedded STATIC ${LIBXLS_SOURCES})
set_target_properties(libxls_embedded PROPERTIES
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)
target_include_directories(libxls_embedded PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libxls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libxls/src
)
target_compile_definitions(libxls_embedded PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
)

# Application Sources
set(SOURCES
    src/main.cpp
    src/database_manager.h
    src/database_manager.cpp
    src/engine/accounting_engine.h
    src/engine/fiscal_year_helper.h
    src/engine/fiscal_year_helper.cpp
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
    src/models/interest_model.h
    src/models/interest_model.cpp
    src/models/ledger_statement_model.h
    src/models/ledger_statement_model.cpp
    src/models/purchase_register_model.h
    src/models/purchase_register_model.cpp
    src/models/sales_register_model.h
    src/models/sales_register_model.cpp
    src/models/stock_register_model.h
    src/models/stock_register_model.cpp
    src/models/milling_statement_model.h
    src/models/milling_statement_model.cpp
    src/services/accounting_date_service.h
    src/services/accounting_date_service.cpp
    src/services/financial_math_service.h
    src/services/financial_math_service.cpp
    src/models/sales_voucher_controller.h
    src/models/sales_voucher_controller.cpp
    src/models/purchase_voucher_controller.h
    src/models/purchase_voucher_controller.cpp
    src/models/journal_voucher_controller.h
    src/models/journal_voucher_controller.cpp
    src/models/cheque_voucher_controller.h
    src/models/cheque_voucher_controller.cpp
    src/models/tds_voucher_controller.h
    src/models/tds_voucher_controller.cpp
    src/models/tax_challan_controller.h
    src/models/tax_challan_controller.cpp
    src/models/tcs_receipt_voucher_controller.h
    src/models/tcs_receipt_voucher_controller.cpp
    src/models/jform_voucher_controller.h
    src/models/jform_voucher_controller.cpp
    src/models/milling_batch_controller.h
    src/models/milling_batch_controller.cpp
    src/models/paddy_procurement_controller.h
    src/models/paddy_procurement_controller.cpp
    src/models/menu_tree_manager.h
    src/models/menu_tree_manager.cpp
    src/models/ledger_master_controller.h
    src/models/ledger_master_controller.cpp
    src/models/stock_master_controller.h
    src/models/stock_master_controller.cpp
    src/services/print_export_controller.h
    src/services/print_export_controller.cpp
    src/models/global_key_filter.h
    src/services/app_keyboard_controller.h
    src/services/app_keyboard_controller.cpp
    src/services/canara_bank_statement_parser.h
    src/services/canara_bank_statement_parser.cpp
    src/services/bank_statement_excel_parser.h
    src/services/bank_statement_excel_parser.cpp
    src/models/bank_statement_controller.h
    src/models/bank_statement_controller.cpp
    src/models/transport_dispatch_controller.h
    src/models/transport_dispatch_controller.cpp
    src/models/debit_credit_note_controller.h
    src/models/debit_credit_note_controller.cpp
    src/engine/bahi_khata_migrator.h
    src/engine/bahi_khata_migrator.cpp
    src/widgets/kbd_badge_button.h
    src/widgets/kbd_badge_button.cpp
    src/widgets/accounting_date_edit.h
    src/widgets/accounting_date_edit.cpp
    src/widgets/account_search_box.h
    src/widgets/account_search_box.cpp
    src/widgets/account_search_delegate.h
    src/widgets/account_search_delegate.cpp
    src/widgets/ledger_table_view.h
    src/widgets/ledger_table_view.cpp
    src/widgets/ledger_statement_widget.h
    src/widgets/ledger_statement_widget.cpp
    src/widgets/accounting_period_dialog.h
    src/widgets/accounting_period_dialog.cpp
    src/engine/balance_sheet_calculator.h
    src/engine/balance_sheet_calculator.cpp
    src/engine/stock_valuation_engine.h
    src/engine/stock_valuation_engine.cpp
    src/models/balance_sheet_controller.h
    src/models/balance_sheet_controller.cpp
    src/printing/balance_sheet_printer.h
    src/printing/balance_sheet_printer.cpp
    src/printing/profit_loss_printer.h
    src/printing/profit_loss_printer.cpp
    src/widgets/balance_sheet_widget.h
    src/widgets/balance_sheet_widget.cpp
    src/engine/profit_loss_calculator.h
    src/engine/profit_loss_calculator.cpp
    src/models/profit_loss_controller.h
    src/models/profit_loss_controller.cpp
    src/widgets/profit_loss_widget.h
    src/widgets/profit_loss_widget.cpp
    src/widgets/custom_closing_stock_widget.h
    src/widgets/custom_closing_stock_widget.cpp
    src/widgets/voucher_common.h
    src/widgets/voucher_date_dialog.h
    src/widgets/voucher_date_dialog.cpp
    src/widgets/custom_dialogs.h
    src/widgets/custom_dialogs.cpp
    src/widgets/party_search_widget.h
    src/widgets/party_search_widget.cpp
    src/widgets/item_search_delegate.h
    src/widgets/item_search_delegate.cpp
    src/widgets/dashboard_widget.h
    src/widgets/dashboard_widget.cpp
    src/widgets/sales_voucher_widget.h
    src/widgets/sales_voucher_widget.cpp
    src/widgets/purchase_voucher_widget.h
    src/widgets/purchase_voucher_widget.cpp
    src/widgets/new_firm_dialog.h
    src/widgets/new_firm_dialog.cpp
    src/widgets/mdb_migration_dialog.h
    src/widgets/mdb_migration_dialog.cpp
    src/widgets/firm_selector_widget.h
    src/widgets/firm_selector_widget.cpp
    src/widgets/cheque_voucher_widget.h
    src/widgets/cheque_voucher_widget.cpp
    src/widgets/journal_voucher_widget.h
    src/widgets/journal_voucher_widget.cpp
    src/models/iform_model.h
    src/models/iform_model.cpp
    src/models/mandi_reports_controller.h
    src/models/mandi_reports_controller.cpp
    src/widgets/new_ledger_widget.h
    src/widgets/new_ledger_widget.cpp
    src/widgets/modify_ledger_widget.h
    src/widgets/modify_ledger_widget.cpp
    src/widgets/jform_voucher_widget.h
    src/widgets/jform_voucher_widget.cpp
    src/widgets/iform_voucher_widget.h
    src/widgets/iform_voucher_widget.cpp
    src/widgets/mandi_reports_widget.h
    src/widgets/mandi_reports_widget.cpp
    src/widgets/day_book_widget.h
    src/widgets/day_book_widget.cpp
    src/widgets/gstr_reports_widget.h
    src/widgets/gstr_reports_widget.cpp
    src/widgets/milling_statement_widget.h
    src/widgets/milling_statement_widget.cpp
    src/widgets/keyboard_shortcuts_dialog.h
    src/widgets/keyboard_shortcuts_dialog.cpp
    src/widgets/print_copy_options_dialog.h
    src/widgets/print_copy_options_dialog.cpp
    src/widgets/weighbridge_kanda_dialog.h
    src/widgets/weighbridge_kanda_dialog.cpp
    src/widgets/item_movement_dialog.h
    src/widgets/item_movement_dialog.cpp
    src/widgets/new_group_widget.h
    src/widgets/new_group_widget.cpp
    src/widgets/modify_group_widget.h
    src/widgets/modify_group_widget.cpp
    src/widgets/new_stock_item_widget.h
    src/widgets/new_stock_item_widget.cpp
    src/widgets/modify_stock_item_widget.h
    src/widgets/modify_stock_item_widget.cpp
    src/widgets/sales_register_widget.h
    src/widgets/sales_register_widget.cpp
    src/widgets/purchase_register_widget.h
    src/widgets/purchase_register_widget.cpp
    src/widgets/stock_detail_widget.h
    src/widgets/stock_detail_widget.cpp
    src/widgets/paddy_procurement_widget.h
    src/widgets/paddy_procurement_widget.cpp
    src/widgets/milling_voucher_widget.h
    src/widgets/milling_voucher_widget.cpp
    src/widgets/tds_voucher_widget.h
    src/widgets/tds_voucher_widget.cpp
    src/widgets/tds_tcs_hub_dialog.h
    src/widgets/tds_tcs_hub_dialog.cpp
    src/widgets/tds_options_dialog.h
    src/widgets/tds_options_dialog.cpp
    src/widgets/tcs_options_dialog.h
    src/widgets/tcs_options_dialog.cpp
    src/widgets/tax_challan_creation_widget.h
    src/widgets/tax_challan_creation_widget.cpp
    src/widgets/tax_challan_register_widget.h
    src/widgets/tax_challan_register_widget.cpp
    src/widgets/tcs_receipt_voucher_widget.h
    src/widgets/tcs_receipt_voucher_widget.cpp
    src/widgets/tcs_receipt_list_widget.h
    src/widgets/tcs_receipt_list_widget.cpp
    src/widgets/tds_vouchers_list_widget.h
    src/widgets/tds_vouchers_list_widget.cpp
    src/widgets/advance_payment_194q_widget.h
    src/widgets/advance_payment_194q_widget.cpp
    src/widgets/advance_payment_194q_list_widget.h
    src/widgets/advance_payment_194q_list_widget.cpp
    src/widgets/form16a_entry_dialog.h
    src/widgets/form16a_entry_dialog.cpp
    src/widgets/form16a_list_widget.h
    src/widgets/form16a_list_widget.cpp
    src/widgets/tds_acknowledgement_dialog.h
    src/widgets/tds_acknowledgement_dialog.cpp
    src/widgets/tcs_config_dialog.h
    src/widgets/tcs_config_dialog.cpp
    src/widgets/navigation_menu_dialog.h
    src/widgets/navigation_menu_dialog.cpp
    src/widgets/interest_calculator_widget.h
    src/widgets/interest_calculator_widget.cpp
    src/widgets/bank_statement_import_widget.h
    src/widgets/bank_statement_import_widget.cpp
    src/widgets/transport_dispatch_widget.h
    src/widgets/transport_dispatch_widget.cpp
    src/widgets/debit_credit_note_widget.h
    src/widgets/debit_credit_note_widget.cpp
    src/engine/mandi_calculator.h
    src/engine/mandi_calculator.cpp
    src/engine/aank_interest_engine.h
    src/engine/aank_interest_engine.cpp
    src/engine/gst_tax_engine.h
    src/engine/gst_tax_engine.cpp
    src/engine/milling_yield_engine.h
    src/engine/milling_yield_engine.cpp
    src/engine/gstr1_engine.h
    src/engine/gstr1_engine.cpp
    src/engine/gstr2_reconciler.h
    src/engine/gstr2_reconciler.cpp
    src/engine/gstr3b_engine.h
    src/engine/gstr3b_engine.cpp
    src/engine/depreciation_calculator.h
    src/engine/depreciation_calculator.cpp
    src/models/trial_balance_controller.h
    src/models/trial_balance_controller.cpp
    src/models/capital_accounts_controller.h
    src/models/capital_accounts_controller.cpp
    src/widgets/trial_balance_widget.h
    src/widgets/trial_balance_widget.cpp
    src/widgets/capital_accounts_widget.h
    src/widgets/capital_accounts_widget.cpp
    src/widgets/depreciation_chart_widget.h
    src/widgets/depreciation_chart_widget.cpp
    src/widgets/joint_reports_dialog.h
    src/widgets/joint_reports_dialog.cpp
    src/models/cash_bank_flow_controller.h
    src/models/cash_bank_flow_controller.cpp
    src/widgets/cash_bank_flow_widget.h
    src/widgets/cash_bank_flow_widget.cpp
    src/widgets/cash_voucher_widget.h
    src/widgets/cash_voucher_widget.cpp
    src/widgets/cashbook_options_dialog.h
    src/widgets/cashbook_options_dialog.cpp
    src/widgets/main_window.h
    src/widgets/main_window.cpp
)

qt_add_executable(${PROJECT_NAME} ${SOURCES})

# Standard Qt 6 QML Module Compilation
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/widgets
    src/engine
    src/models
    src/services
    third_party/sqlite
    third_party/libmdb
    third_party/miniz
    third_party/libxls/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_LIBMDB=1)

target_link_libraries(${PROJECT_NAME} PRIVATE
    sqlite3_embedded
    libmdb_embedded
    miniz_embedded
    libxls_embedded
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::Svg
    Qt6::Widgets
    Qt6::PrintSupport
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE iconv)
endif()

# Set Target Properties for macOS and Windows
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Mahadev Rice Mill ERP"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mahadevagro.ricemill"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.2.0"
    MACOSX_BUNDLE_VERSION "0.2.0"
    WIN32_EXECUTABLE $<IF:$<CONFIG:Debug>,FALSE,TRUE>
)

# ==============================================================================
# Native LogicBoard Automated Test Suite
# ==============================================================================
enable_testing()

set(TEST_SOURCES
    tests/test_logicboard_suite.cpp
    src/database_manager.h
    src/database_manager.cpp
    src/services/accounting_date_service.h
    src/services/accounting_date_service.cpp
    src/services/financial_math_service.h
    src/services/financial_math_service.cpp
    src/models/base_table_model.h
    src/models/base_table_model.cpp
    src/models/sales_voucher_controller.h
    src/models/sales_voucher_controller.cpp
    src/models/purchase_voucher_controller.h
    src/models/purchase_voucher_controller.cpp
    src/models/journal_voucher_controller.h
    src/models/journal_voucher_controller.cpp
    src/models/cheque_voucher_controller.h
    src/models/cheque_voucher_controller.cpp
    src/models/tds_voucher_controller.h
    src/models/tds_voucher_controller.cpp
    src/models/tax_challan_controller.h
    src/models/tax_challan_controller.cpp
    src/models/tcs_receipt_voucher_controller.h
    src/models/tcs_receipt_voucher_controller.cpp
    src/models/jform_voucher_controller.h
    src/models/jform_voucher_controller.cpp
    src/models/milling_batch_controller.h
    src/models/milling_batch_controller.cpp
    src/models/paddy_procurement_controller.h
    src/models/paddy_procurement_controller.cpp
    src/models/ledger_master_controller.h
    src/models/ledger_master_controller.cpp
    src/models/stock_master_controller.h
    src/models/stock_master_controller.cpp
    src/models/parties_model.h
    src/models/parties_model.cpp
    src/models/sales_model.h
    src/models/sales_model.cpp
    src/models/purchase_model.h
    src/models/purchase_model.cpp
    src/models/vouchers_model.h
    src/models/vouchers_model.cpp
    src/models/stock_items_model.h
    src/models/stock_items_model.cpp
    src/models/paddy_arrivals_model.h
    src/models/paddy_arrivals_model.cpp
    src/models/jform_model.h
    src/models/jform_model.cpp
    src/models/tds_model.h
    src/models/tds_model.cpp
    src/models/interest_model.h
    src/models/interest_model.cpp
    src/models/milling_model.h
    src/models/milling_model.cpp
    src/models/account_groups_model.h
    src/models/account_groups_model.cpp
    src/models/ledger_statement_model.h
    src/models/ledger_statement_model.cpp
    src/models/purchase_register_model.h
    src/models/purchase_register_model.cpp
    src/models/sales_register_model.h
    src/models/sales_register_model.cpp
    src/models/stock_register_model.h
    src/models/stock_register_model.cpp
    src/models/milling_statement_model.h
    src/models/milling_statement_model.cpp
    src/models/dashboard_controller.h
    src/models/dashboard_controller.cpp
    src/models/firm_manager.h
    src/models/firm_manager.cpp
    src/models/financial_years_model.h
    src/models/financial_years_model.cpp
    src/engine/accounting_engine.h
    src/engine/fiscal_year_helper.h
    src/engine/fiscal_year_helper.cpp
    src/services/print_export_controller.h
    src/services/print_export_controller.cpp
    src/services/canara_bank_statement_parser.h
    src/services/canara_bank_statement_parser.cpp
    src/services/bank_statement_excel_parser.h
    src/services/bank_statement_excel_parser.cpp
    src/models/bank_statement_controller.h
    src/models/bank_statement_controller.cpp
    src/models/transport_dispatch_controller.h
    src/models/transport_dispatch_controller.cpp
    src/models/debit_credit_note_controller.h
    src/models/debit_credit_note_controller.cpp
    src/engine/balance_sheet_calculator.h
    src/engine/balance_sheet_calculator.cpp
    src/engine/stock_valuation_engine.h
    src/engine/stock_valuation_engine.cpp
    src/models/balance_sheet_controller.h
    src/models/balance_sheet_controller.cpp
    src/engine/profit_loss_calculator.h
    src/engine/profit_loss_calculator.cpp
    src/models/profit_loss_controller.h
    src/models/profit_loss_controller.cpp
    src/printing/balance_sheet_printer.h
    src/printing/balance_sheet_printer.cpp
    src/printing/profit_loss_printer.h
    src/printing/profit_loss_printer.cpp
    src/engine/bahi_khata_migrator.h
    src/engine/bahi_khata_migrator.cpp
    src/models/iform_model.h
    src/models/iform_model.cpp
    src/models/mandi_reports_controller.h
    src/models/mandi_reports_controller.cpp
    src/engine/depreciation_calculator.h
    src/engine/depreciation_calculator.cpp
    src/models/trial_balance_controller.h
    src/models/trial_balance_controller.cpp
    src/models/capital_accounts_controller.h
    src/models/capital_accounts_controller.cpp
    src/models/cash_bank_flow_controller.h
    src/models/cash_bank_flow_controller.cpp
)

add_executable(LogicBoardTestSuite ${TEST_SOURCES})
target_include_directories(LogicBoardTestSuite PRIVATE
    src
    src/engine
    src/models
    src/services
    src/printing
    third_party/sqlite
    third_party/libmdb
    third_party/miniz
    third_party/libxls/include
)
target_compile_definitions(LogicBoardTestSuite PRIVATE HAS_LIBMDB=1)
target_link_libraries(LogicBoardTestSuite PRIVATE
    sqlite3_embedded
    libmdb_embedded
    miniz_embedded
    libxls_embedded
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Test
)

if(APPLE)
    target_link_libraries(LogicBoardTestSuite PRIVATE iconv)
endif()
add_test(NAME LogicBoardTestSuite COMMAND LogicBoardTestSuite)

# Engines & Compliance Test Suite
set(ENGINE_TEST_SOURCES
    tests/test_engines.cpp
    src/engine/mandi_calculator.h
    src/engine/mandi_calculator.cpp
    src/engine/aank_interest_engine.h
    src/engine/aank_interest_engine.cpp
    src/engine/gst_tax_engine.h
    src/engine/gst_tax_engine.cpp
    src/engine/milling_yield_engine.h
    src/engine/milling_yield_engine.cpp
    src/engine/gstr1_engine.h
    src/engine/gstr1_engine.cpp
    src/engine/gstr2_reconciler.h
    src/engine/gstr2_reconciler.cpp
    src/engine/gstr3b_engine.h
    src/engine/gstr3b_engine.cpp
    src/database_manager.h
    src/database_manager.cpp
)

add_executable(EnginesTestSuite ${ENGINE_TEST_SOURCES})
target_include_directories(EnginesTestSuite PRIVATE
    src
    src/engine
    src/models
    src/services
    third_party/sqlite
    third_party/libmdb
    third_party/miniz
    third_party/libxls/include
)
target_compile_definitions(EnginesTestSuite PRIVATE HAS_LIBMDB=1)
target_link_libraries(EnginesTestSuite PRIVATE
    sqlite3_embedded
    libmdb_embedded
    miniz_embedded
    libxls_embedded
    Qt6::Core
    Qt6::Gui
    Qt6::Sql
    Qt6::Widgets
    Qt6::Test
)
if(APPLE)
    target_link_libraries(EnginesTestSuite PRIVATE iconv)
endif()
add_test(NAME EnginesTestSuite COMMAND EnginesTestSuite)


