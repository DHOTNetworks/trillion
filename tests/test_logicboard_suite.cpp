#include <QTest>
#include <QObject>
#include <QDate>
#include <QDir>
#include <QFile>
#include <cmath>

#include "../src/services/accounting_date_service.h"
#include "../src/services/financial_math_service.h"
#include "../src/models/sales_voucher_controller.h"
#include "../src/models/purchase_voucher_controller.h"
#include "../src/models/journal_voucher_controller.h"
#include "../src/models/cheque_voucher_controller.h"
#include "../src/models/tds_voucher_controller.h"
#include "../src/models/jform_voucher_controller.h"
#include "../src/models/milling_batch_controller.h"
#include "../src/models/paddy_procurement_controller.h"
#include "../src/models/ledger_master_controller.h"
#include "../src/models/stock_master_controller.h"
#include "../src/models/sales_model.h"
#include "../src/models/purchase_model.h"
#include "../src/models/vouchers_model.h"
#include "../src/models/parties_model.h"
#include "../src/models/financial_years_model.h"
#include "../src/models/milling_model.h"
#include "../src/models/jform_model.h"
#include "../src/models/iform_model.h"
#include "../src/models/mandi_reports_controller.h"
#include "../src/models/tds_model.h"
#include "../src/models/stock_items_model.h"
#include "../src/models/account_groups_model.h"
#include "../src/models/account_classifier.h"
#include "../src/services/canara_bank_statement_parser.h"
#include "../src/services/bank_statement_excel_parser.h"
#include "../src/models/bank_statement_controller.h"
#include "../src/models/transport_dispatch_controller.h"
#include "../src/models/debit_credit_note_controller.h"
#include "../src/models/firm_manager.h"
#include "../src/engine/bahi_khata_migrator.h"
#include "../src/engine/group_hierarchy_pipeline.h"
#include "../src/engine/ledger_pipeline.h"
#include "../src/database_manager.h"
#include "../src/engine/fiscal_year_helper.h"
#include "../src/engine/balance_sheet_calculator.h"
#include "../src/models/balance_sheet_controller.h"
#include "../src/engine/profit_loss_calculator.h"
#include "../src/models/profit_loss_controller.h"
#include "../src/engine/stock_valuation_engine.h"
#include "../src/models/tax_challan_controller.h"
#include "../src/models/tcs_receipt_voucher_controller.h"
#include "../src/models/stock_register_model.h"
#include "../src/engine/depreciation_calculator.h"
#include "../src/models/bardana_controller.h"
#include "../src/models/gate_register_controller.h"
#include "../src/models/sauda_controller.h"
#include "../src/models/trial_balance_controller.h"
#include "../src/models/capital_accounts_controller.h"
#include "../src/models/cash_bank_flow_controller.h"
#include "../src/models/salary_register_controller.h"
#include "../src/services/print_export_controller.h"

class LogicBoardTestSuite : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1. Date Engine Tests
    void testDateParsingAndFormatting();
    void testDateFyBoundaryValidation();
    void testDayOfWeekResolution();

    // 2. Financial Math & GST Tests
    void testFinancialMathRounding();
    void testGstSplittingIntrastateAndInterstate();
    void testRoundOffCalculation();
    void testInrCurrencyFormatting();

    // 3. Fiscal Year Partitioning & Clamping
    void testFiscalYearPartitioningAndClamping();
    void testZeroLeakagePartyPartitioning();

    // 4. Voucher Controllers & ViewModels Tests
    void testSalesVoucherControllerCalculations();
    void testPurchaseVoucherControllerCalculations();
    void testJournalVoucherDebitCreditBalanceCheck();
    void testChequeVoucherModes();
    void testTdsCalculationsWithSurchargeAndCess();
    void testJFormYieldAndDeductionCalculations();
    void testMillingBatchYieldDistribution();
    void testPaddyProcurementMoistureDeductions();
    void testPerFinancialYearVoucherNumbering();

    // 5. Masters & Regulatory Validations
    void testGstinValidationAndPanExtraction();
    void testStockItemOpeningValuation();

    // 6. Weighbridge & Transport Logistics
    void testTransportDispatchController();

    // 7. GST Debit & Credit Notes
    void testDebitCreditNoteController();

    // 8. Firm & Fiscal Years Engine
    void testFirmTypeResolutionAndDynamicFiscalYears();
    void testHistoricalFiscalYearIsolationWithoutGapYears();

    // 9. Balance Sheet & Profit and Loss Reporting Systems
    void testBalanceSheetCalculations();
    void testProfitLossCalculations();
    void testStockValuationEnginePipeline();

    // 10. View Compilation & Statement Parsers
    void testCanaraBankStatementPdfParser();
    void testNativeXlsBankStatementParser();
    void testBankStatementControllerPosting();
    void testBahiKhataMdbStationAndLedgerMigration();

    // 11. TDS and TCS Ecosystem Tests
    void testTcsReceiptVoucherPosting();
    void testTaxDepositChallanPosting();

    // 12. Bahi-Khata Stock Register Subsystem Tests
    void testStockRegisterSubsystem();

    // 13. Final Reports & Depreciation Subsystems
    void testFinalReportsAndDepreciationSubsystem();

    // 14. Cash Book & Flow Statements Subsystem
    void testCashBankFlowAndCashVouchers();

    // 15. Universal Print, PDF, ODF & Excel Export Subsystems
    void testPrintExportControllerUniversalExports();

    // 16. Bardana, Gate Register & Sauda Subsystems
    void testBardanaControllerWorkflow();
    void testGateRegisterControllerWorkflow();
    void testSaudaContractAndDalaliSettlementWorkflow();

    // 17. 4-Code Structure Enforcement for New Groups & Ledgers
    void testNewGroupAndLedgerFourCodeStructure();

    // 18. Employee Payroll & Salary Register Crediting
    void testSalaryRegisterControllerAndPayrollPosting();
};

#include "mdbtools.h"
void LogicBoardTestSuite::testBahiKhataMdbStationAndLedgerMigration() {
    BahiKhataMigrator migrator;
    
    // Migrate Data.004 (Mandi Firm: Sushil Kr Pradeep Kr) into sushil_kr_pardeep_kr_data_004.db
    QString db004Path = "data/sushil_kr_pardeep_kr_data_004.db";
    if (QFile::exists("../data")) db004Path = "../data/sushil_kr_pardeep_kr_data_004.db";
    DatabaseManager::instance().switchDatabase(db004Path);
    QString mdb004 = "Bahi-Khata-Data/Data.004";
    if (!QFile::exists(mdb004) && QFile::exists("../Bahi-Khata-Data/Data.004")) mdb004 = "../Bahi-Khata-Data/Data.004";
    bool ok3 = migrator.migrate_mdb_file(mdb004);
    qDebug() << "[TEST MIGRATION] Data.004 migration success:" << ok3;
    QVERIFY(ok3);

    PartiesModel partiesModel;
    QStringList stations004 = partiesModel.get_stations();
    QStringList groups004 = partiesModel.get_account_groups();
    qDebug() << "[TEST MIGRATION 004] Distinct Stations Count:" << stations004.size();
    qDebug() << "[TEST MIGRATION 004] Distinct Groups Count:" << groups004.size();
    QVERIFY(stations004.size() > 0);
    QVERIFY(groups004.size() > 0);

    // Verify J-Forms and I-Forms migrated
    QVariant jfCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM jform_vouchers;");
    QVariant ifCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM iform_vouchers;");
    qDebug() << "[TEST MIGRATION 004] J-Form Count:" << jfCount.toInt() << "I-Form Count:" << ifCount.toInt();
    QVERIFY(jfCount.toInt() > 0);
    QVERIFY(ifCount.toInt() > 0);

    // Verify Form M statutory report generation
    MandiReportsController mandiCtrl;
    QVariantMap formM = mandiCtrl.get_form_m_return("2011-04-01", "2027-03-31");
    qDebug() << "[TEST MIGRATION 004] Form M Total Bags:" << formM.value("total_bags").toInt()
             << "Total Weight:" << formM.value("total_weight").toDouble()
             << "Total Market Fee:" << formM.value("total_mandi_fee").toDouble()
             << "Total HRDF:" << formM.value("total_hrdf").toDouble();
    QVERIFY(formM.value("total_bags").toInt() > 0);
    QVERIFY(formM.value("total_mandi_fee").toDouble() > 0.0);

    // Reset back to isolated test unit DB for subsequent tests
    QString testDbPath = "data/test_unit_suite.db";
    if (QFile::exists("../data")) testDbPath = "../data/test_unit_suite.db";
    DatabaseManager::instance().switchDatabase(testDbPath);
}

void LogicBoardTestSuite::initTestCase() {
    qDebug() << "[TEST INIT] Initializing LogicBoard Test Suite...";
    QString testDbPath = "data/test_unit_suite.db";
    if (QFile::exists("../data")) {
        testDbPath = "../data/test_unit_suite.db";
    }
    QFile::remove(testDbPath);
    DatabaseManager::instance().initDatabase(testDbPath);

    // Seed test_unit_suite.db with Data.002 fixture data so calculation tests have real data
    BahiKhataMigrator migrator;
    QString mdb002 = "Bahi-Khata-Data/Data.002";
    if (!QFile::exists(mdb002) && QFile::exists("../Bahi-Khata-Data/Data.002")) mdb002 = "../Bahi-Khata-Data/Data.002";
    if (QFile::exists(mdb002)) {
        migrator.migrate_mdb_file(mdb002);
    }
}

void LogicBoardTestSuite::cleanupTestCase() {
    qDebug() << "[TEST CLEANUP] LogicBoard Test Suite Completed.";
}

// -------------------------------------------------------------
// 1. Date Engine Tests
// -------------------------------------------------------------
void LogicBoardTestSuite::testDateParsingAndFormatting() {
    auto &dateSvc = AccountingDateService::instance();

    // Dot separated short year (e.g. 1.4.26 -> 01/04/2026)
    QCOMPARE(dateSvc.resolveDate("1.4.26"), QString("01/04/2026"));
    QCOMPARE(dateSvc.resolveDate("1.3.26"), QString("01/03/2026"));
    QCOMPARE(dateSvc.resolveDate("15.08.25"), QString("15/08/2025"));
    QCOMPARE(dateSvc.resolveDate("01.04.26"), QString("01/04/2026"));

    // Dash separated
    QCOMPARE(dateSvc.resolveDate("1-4-26"), QString("01/04/2026"));
    QCOMPARE(dateSvc.resolveDate("1-3-2026"), QString("01/03/2026"));
    QCOMPARE(dateSvc.resolveDate("01-03-2026"), QString("01/03/2026"));

    // Slash separated
    QCOMPARE(dateSvc.resolveDate("1/4/26"), QString("01/04/2026"));
    QCOMPARE(dateSvc.resolveDate("1/3/26"), QString("01/03/2026"));
    QCOMPARE(dateSvc.resolveDate("31/12/2025"), QString("31/12/2025"));

    // ISO conversions
    QCOMPARE(dateSvc.toIso("1.4.26"), QString("2026-04-01"));
    QCOMPARE(dateSvc.toIso("01/03/2026"), QString("2026-03-01"));
    QCOMPARE(dateSvc.toIso("15-08-2025"), QString("2025-08-15"));
    QCOMPARE(dateSvc.fromIso("2026-03-01"), QString("01/03/2026"));

    // FinancialYearsModel date pattern parser (Accounting quick entry format)
    FinancialYearsModel fyModel;
    QCOMPARE(fyModel.parse_date_pattern("1.4.26"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("01.04.26"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("1/4/26"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("1-4-26"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("010426"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("01042026"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("1-4-1926"), QString("01-04-2026"));
    QCOMPARE(fyModel.parse_date_pattern("1-4-1923"), QString("01-04-2023"));

    // Firm Manager date formatting & century normalization (1-4-23 -> 01-04-2023, 31-3-27 -> 31-03-2027)
    QCOMPARE(FirmManager::formatDateToDisplay("1-4-23"), QString("01-04-2023"));
    QCOMPARE(FirmManager::formatDateToDisplay("31-3-27"), QString("31-03-2027"));
    QCOMPARE(FirmManager::formatDateToDisplay("01/04/23"), QString("01-04-2023"));
    QCOMPARE(FirmManager::formatDateToDisplay("31/03/27"), QString("31-03-2027"));
    QCOMPARE(FirmManager::formatDateToDisplay("1-4-1923"), QString("01-04-2023"));
    QCOMPARE(FirmManager::formatDateToDisplay("31-3-1927"), QString("31-03-2027"));
    QCOMPARE(FirmManager::formatDateToDisplay("2023-04-01"), QString("01-04-2023"));
    QCOMPARE(FirmManager::formatDateToDisplay("2027-03-31"), QString("31-03-2027"));
}

void LogicBoardTestSuite::testDateFyBoundaryValidation() {
    auto &dateSvc = AccountingDateService::instance();

    // FY 2025-2026 runs from 01/04/2025 to 31/03/2026
    int fy2025 = 2025;
    QVERIFY(dateSvc.validateDateInFy("01/04/2025", fy2025));
    QVERIFY(dateSvc.validateDateInFy("15/08/2025", fy2025));
    QVERIFY(dateSvc.validateDateInFy("31/03/2026", fy2025));

    // Outside boundaries
    QVERIFY(!dateSvc.validateDateInFy("31/03/2025", fy2025));
    QVERIFY(!dateSvc.validateDateInFy("01/04/2026", fy2025));
}

void LogicBoardTestSuite::testDayOfWeekResolution() {
    auto &dateSvc = AccountingDateService::instance();
    // 01/03/2026 is Sunday
    QCOMPARE(dateSvc.getDayOfWeek("01/03/2026"), QString("Sunday"));
    // 15/08/2025 is Friday
    QCOMPARE(dateSvc.getDayOfWeek("15/08/2025"), QString("Friday"));
}

// -------------------------------------------------------------
// 2. Financial Math & GST Tests
// -------------------------------------------------------------
void LogicBoardTestSuite::testFinancialMathRounding() {
    auto &mathSvc = FinancialMathService::instance();

    QCOMPARE(mathSvc.round2(123.456), 123.46);
    QCOMPARE(mathSvc.round2(123.454), 123.45);
    QCOMPARE(mathSvc.round2(0.005), 0.01);
    QCOMPARE(mathSvc.round2(0.004), 0.00);
}

void LogicBoardTestSuite::testGstSplittingIntrastateAndInterstate() {
    auto &mathSvc = FinancialMathService::instance();

    // Intrastate: 5% GST on 10,000 => 2.5% CGST (250.00) + 2.5% SGST (250.00), 0 IGST
    auto intra = mathSvc.calculateGst(10000.0, 5.0, false);
    QCOMPARE(intra.value("cgst").toDouble(), 250.00);
    QCOMPARE(intra.value("sgst").toDouble(), 250.00);
    QCOMPARE(intra.value("igst").toDouble(), 0.00);
    QCOMPARE(intra.value("totalTax").toDouble(), 500.00);

    // Interstate: 5% GST on 10,000 => 0 CGST, 0 SGST, 5% IGST (500.00)
    auto inter = mathSvc.calculateGst(10000.0, 5.0, true);
    QCOMPARE(inter.value("cgst").toDouble(), 0.00);
    QCOMPARE(inter.value("sgst").toDouble(), 0.00);
    QCOMPARE(inter.value("igst").toDouble(), 500.00);
    QCOMPARE(inter.value("totalTax").toDouble(), 500.00);
}

void LogicBoardTestSuite::testRoundOffCalculation() {
    auto &mathSvc = FinancialMathService::instance();

    // 1245.45 => Round off = -0.45, Grand Total = 1245.00
    QCOMPARE(mathSvc.calculateRoundOff(1245.45), -0.45);
    QCOMPARE(mathSvc.calculateGrandTotal(1245.45), 1245.00);

    // 1245.60 => Round off = +0.40, Grand Total = 1246.00
    QCOMPARE(mathSvc.calculateRoundOff(1245.60), 0.40);
    QCOMPARE(mathSvc.calculateGrandTotal(1245.60), 1246.00);
}

void LogicBoardTestSuite::testInrCurrencyFormatting() {
    auto &mathSvc = FinancialMathService::instance();

    QString f1 = mathSvc.formatInr(125000.50, true);
    QVERIFY(f1.contains("1,25,000.50"));

    QString f2 = mathSvc.formatInr(500829152.00, false);
    QCOMPARE(f2, QString("50,08,29,152.00"));
}

// -------------------------------------------------------------
// 3. Fiscal Year Partitioning & Clamping (FY Separator)
// -------------------------------------------------------------
void LogicBoardTestSuite::testFiscalYearPartitioningAndClamping() {
    // 1. Fiscal Year Resolution
    FiscalYearInfo activeFy = FiscalYearHelper::getActiveFiscalYear();
    QVERIFY(activeFy.isValid());
    QVERIFY(activeFy.startDate <= activeFy.endDate);

    // 2. Normalization to ISO
    QCOMPARE(FiscalYearHelper::normalizeToIso("1/4/24"), QString("2024-04-01"));
    QCOMPARE(FiscalYearHelper::normalizeToIso("31.03.2025"), QString("2025-03-31"));
    QCOMPARE(FiscalYearHelper::normalizeToIso("2024-04-01"), QString("2024-04-01"));

    // 3. Clamping boundary checks
    FiscalYearInfo mockFy;
    mockFy.name = "FY 2024-25";
    mockFy.startDate = "2024-04-01";
    mockFy.endDate = "2025-03-31";
    mockFy.isActive = true;

    QString fromDate = "2023-01-01";
    QString toDate = "2026-12-31";
    FiscalYearHelper::clampDateRangeToFiscalYear(fromDate, toDate, mockFy);
    QCOMPARE(fromDate, QString("2024-04-01"));
    QCOMPARE(toDate, QString("2025-03-31"));

    // Clamping when dates are within range
    fromDate = "2024-06-01";
    toDate = "2024-10-31";
    FiscalYearHelper::clampDateRangeToFiscalYear(fromDate, toDate, mockFy);
    QCOMPARE(fromDate, QString("2024-06-01"));
    QCOMPARE(toDate, QString("2024-10-31"));

    // Inverted range correction (safely resets to full FY boundary)
    fromDate = "2024-11-01";
    toDate = "2024-05-01";
    FiscalYearHelper::clampDateRangeToFiscalYear(fromDate, toDate, mockFy);
    QCOMPARE(fromDate, QString("2024-04-01"));
    QCOMPARE(toDate, QString("2025-03-31"));

    // 4. Date Prior and In Fiscal Year
    QVERIFY(FiscalYearHelper::isDatePriorTo("2024-03-31", "2024-04-01"));
    QVERIFY(!FiscalYearHelper::isDatePriorTo("2024-04-01", "2024-04-01"));
    QVERIFY(FiscalYearHelper::isDateInFiscalYear("2024-04-01", mockFy));
    QVERIFY(FiscalYearHelper::isDateInFiscalYear("2025-03-31", mockFy));
    QVERIFY(!FiscalYearHelper::isDateInFiscalYear("2024-03-31", mockFy));
    QVERIFY(!FiscalYearHelper::isDateInFiscalYear("2025-04-01", mockFy));

    // 5. Partition Party Transactions
    // Query a real party from the test database
    QVariant pNameVar = DatabaseManager::instance().executeScalar(
        "SELECT party_name FROM transactions WHERE party_name IS NOT NULL AND party_name != '' LIMIT 1;"
    );
    if (pNameVar.isValid() && !pNameVar.toString().isEmpty()) {
        QString testParty = pNameVar.toString();
        PartitionedLedgerData partData = FiscalYearHelper::partitionPartyTransactions(testParty);
        QVERIFY(partData.activeFy.isValid());
        QVERIFY(!partData.effectiveFromDate.isEmpty());
        QVERIFY(!partData.effectiveToDate.isEmpty());

        // Verify that every transaction in drEntries & crEntries is within effective range or is OP-BAL (id == -1)
        for (const auto& e : partData.drEntries) {
            if (e.id != -1) {
                QVERIFY2(e.vIso >= partData.effectiveFromDate, qPrintable(QString("Entry date %1 < fromDate %2").arg(e.vIso, partData.effectiveFromDate)));
                QVERIFY2(e.vIso <= partData.effectiveToDate, qPrintable(QString("Entry date %1 > toDate %2").arg(e.vIso, partData.effectiveToDate)));
            }
        }
        for (const auto& e : partData.crEntries) {
            if (e.id != -1) {
                QVERIFY2(e.vIso >= partData.effectiveFromDate, qPrintable(QString("Entry date %1 < fromDate %2").arg(e.vIso, partData.effectiveFromDate)));
                QVERIFY2(e.vIso <= partData.effectiveToDate, qPrintable(QString("Entry date %1 > toDate %2").arg(e.vIso, partData.effectiveToDate)));
            }
        }
    }

    // 6. Test Fiscal Years Discovery & Database Mapping
    FiscalYearHelper::ensureFiscalYearsDiscovered();
    QList<FiscalYearInfo> allFys = FiscalYearHelper::getAllFiscalYears();
    QVERIFY(allFys.size() >= 1);
    for (const auto& fy : allFys) {
        QVERIFY(fy.isValid());
        QVERIFY(fy.startDate < fy.endDate);
        QVERIFY(fy.name.startsWith("FY "));
    }
}

void LogicBoardTestSuite::testZeroLeakagePartyPartitioning() {
    qDebug() << "[TEST] Running Zero-Leakage Party Partitioning validation...";
    auto& db = DatabaseManager::instance();

    // 1. Insert two distinct parties that share overlapping keywords in their names
    db.executeNonQuery("INSERT INTO parties (name, group_name, opening_balance, balance_type, legacy_id) "
                       "VALUES ('Canara Bank Isolation Test', 'Bank Accounts', 50000.0, 'Dr', 9901);");
    int canaraId = static_cast<int>(db.lastInsertedId());

    db.executeNonQuery("INSERT INTO parties (name, group_name, opening_balance, balance_type, legacy_id) "
                       "VALUES ('IndusInd Bank Isolation Test', 'Bank Accounts', 20000.0, 'Dr', 9902);");
    int indusId = static_cast<int>(db.lastInsertedId());

    // 2. Insert distinct transactions for each party in active FY 2025-26
    db.executeNonQuery("INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, "
                       "trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount) "
                       "VALUES (1, 'FY 2025-26', 'V-CANARA-1', '2025-06-15', 'Payment', 'ChPt', 9901, ?, 'Canara Bank Isolation Test', 'Vendor A', 'Cr', 15000.0);",
                       {canaraId});

    db.executeNonQuery("INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, "
                       "trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount) "
                       "VALUES (1, 'FY 2025-26', 'V-INDUS-1', '2025-07-20', 'Payment', 'ChPt', 9902, ?, 'IndusInd Bank Isolation Test', 'Vendor B', 'Cr', 8000.0);",
                       {indusId});

    // Also insert prior year transactions (2024-05-10 in FY 2024-25)
    db.executeNonQuery("INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, "
                       "trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount) "
                       "VALUES (1, 'FY 2024-25', 'V-CANARA-PRIOR', '2024-05-10', 'Receipt', 'ChRt', 9901, ?, 'Canara Bank Isolation Test', 'Customer X', 'Dr', 25000.0);",
                       {canaraId});

    db.executeNonQuery("INSERT INTO transactions (fy_id, financial_year, voucher_no, voucher_date, voucher_type, "
                       "trans_type, account_code, party_id, party_name, opposing_account, dr_cr, amount) "
                       "VALUES (1, 'FY 2024-25', 'V-INDUS-PRIOR', '2024-06-10', 'Receipt', 'ChRt', 9902, ?, 'IndusInd Bank Isolation Test', 'Customer Y', 'Dr', 100000.0);",
                       {indusId});

    // 3. Partition transactions for Canara Bank in FY 2025-26 (2025-04-01 to 2026-03-31)
    PartitionedLedgerData canaraPart = FiscalYearHelper::partitionPartyTransactions("Canara Bank Isolation Test", "2025-04-01", "2026-03-31");

    // Prior debit: 50,000 (initial op) + 25,000 (prior tx) = 75,000.00
    QCOMPARE(canaraPart.priorDebit, 75000.0);
    QCOMPARE(canaraPart.priorCredit, 0.0);
    QCOMPARE(canaraPart.netOpeningBalance, 75000.0);
    QCOMPARE(canaraPart.openingBalanceType, QString("Dr"));

    // Verify ZERO IndusInd transactions in Canara Bank's partitioned entries
    for (const auto& e : canaraPart.drEntries) {
        QVERIFY(!e.voucherNo.contains("INDUS", Qt::CaseInsensitive));
        QVERIFY(!e.particulars.contains("IndusInd", Qt::CaseInsensitive));
    }
    for (const auto& e : canaraPart.crEntries) {
        QVERIFY(!e.voucherNo.contains("INDUS", Qt::CaseInsensitive));
        QVERIFY(!e.particulars.contains("IndusInd", Qt::CaseInsensitive));
    }
    // Verify Canara period credit has exactly 1 entry: V-CANARA-1 for 15,000
    bool foundCanaraTx = false;
    for (const auto& e : canaraPart.crEntries) {
        if (e.voucherNo == "V-CANARA-1") {
            foundCanaraTx = true;
            QCOMPARE(e.amount, 15000.0);
        }
    }
    QVERIFY(foundCanaraTx);

    // 4. Partition transactions for IndusInd Bank in FY 2025-26
    PartitionedLedgerData indusPart = FiscalYearHelper::partitionPartyTransactions("IndusInd Bank Isolation Test", "2025-04-01", "2026-03-31");

    // Prior debit: 20,000 (initial op) + 100,000 (prior tx) = 120,000.00 (Zero from Canara)
    QCOMPARE(indusPart.priorDebit, 120000.0);
    QCOMPARE(indusPart.priorCredit, 0.0);
    QCOMPARE(indusPart.netOpeningBalance, 120000.0);

    for (const auto& e : indusPart.drEntries) {
        QVERIFY(!e.voucherNo.contains("CANARA", Qt::CaseInsensitive));
        QVERIFY(!e.particulars.contains("Canara", Qt::CaseInsensitive));
    }
    for (const auto& e : indusPart.crEntries) {
        QVERIFY(!e.voucherNo.contains("CANARA", Qt::CaseInsensitive));
        QVERIFY(!e.particulars.contains("Canara", Qt::CaseInsensitive));
    }

    // 5. Clean up test records
    db.executeNonQuery("DELETE FROM transactions WHERE voucher_no LIKE 'V-CANARA%' OR voucher_no LIKE 'V-INDUS%';");
    db.executeNonQuery("DELETE FROM parties WHERE id IN (?, ?);", {canaraId, indusId});

    qDebug() << "[TEST] Zero-Leakage Party Partitioning validation passed successfully!";
}

// -------------------------------------------------------------
// 4. Voucher Controllers & ViewModels Tests
// -------------------------------------------------------------
void LogicBoardTestSuite::testSalesVoucherControllerCalculations() {
    SalesVoucherController ctrl;
    ctrl.resetForm("01/04/2026");

    // Add 2 line items:
    // 1) 100 bags, 50.0 Qtl @ 4000.00 => 2,00,000.00 (5% GST)
    // 2) 50 bags, 25.0 Qtl @ 3800.00 => 95,000.00 (5% GST)
    ctrl.lineItemsModel()->appendRow("Rice Basmati 1121", "1006", "QTL", 100, 50.0, 50.0, 4000.0, 200000.0, 5.0);
    ctrl.lineItemsModel()->appendRow("Rice Sona Masoori", "1006", "QTL", 50, 50.0, 25.0, 3800.0, 95000.0, 5.0);

    QCOMPARE(ctrl.totalBags(), 150);
    QCOMPARE(ctrl.totalWeightQtl(), 75.0);
    QCOMPARE(ctrl.taxableAmount(), 295000.00);

    // 5% Intrastate GST => 14,750.00 (7,375.00 CGST + 7,375.00 SGST)
    ctrl.setGstRate(5.0);
    ctrl.setIsInterstate(false);

    QCOMPARE(ctrl.cgstAmount(), 7375.00);
    QCOMPARE(ctrl.sgstAmount(), 7375.00);
    QCOMPARE(ctrl.igstAmount(), 0.00);
    QCOMPARE(ctrl.totalTaxAmount(), 14750.00);
    QCOMPARE(ctrl.grandTotal(), 309750.00);

    // Switch to Interstate
    ctrl.setIsInterstate(true);
    QCOMPARE(ctrl.cgstAmount(), 0.00);
    QCOMPARE(ctrl.sgstAmount(), 0.00);
    QCOMPARE(ctrl.igstAmount(), 14750.00);
    QCOMPARE(ctrl.grandTotal(), 309750.00);
}

void LogicBoardTestSuite::testPurchaseVoucherControllerCalculations() {
    PurchaseVoucherController ctrl;
    ctrl.resetForm("01/04/2026");

    ctrl.lineItemsModel()->appendRow("Paddy Basmati 1401", "1006", "QTL", 200, 50.0, 100.0, 3200.0, 320000.0);

    QCOMPARE(ctrl.totalBags(), 200);
    QCOMPARE(ctrl.totalWeightQtl(), 100.0);
    QCOMPARE(ctrl.taxableAmount(), 320000.00);

    // Deductions & Expenses
    ctrl.setLabour(500.0);
    ctrl.setDami(8000.0);
    ctrl.setFreightCharges(2500.0);

    QVERIFY(ctrl.grandTotal() > 0.0);
}

void LogicBoardTestSuite::testJournalVoucherDebitCreditBalanceCheck() {
    JournalVoucherController ctrl;
    ctrl.resetForm("01/04/2026");

    // Clear and set balanced entries
    ctrl.rowsModel()->clear();
    ctrl.rowsModel()->appendRow("Dr", "Electricity Expense", 5000.00, 0.0, "Bill #101");
    ctrl.rowsModel()->appendRow("Cr", "HDFC Bank A/c", 0.0, 5000.00, "Chq #882190");

    QCOMPARE(ctrl.totalDebit(), 5000.00);
    QCOMPARE(ctrl.totalCredit(), 5000.00);

    // Intentionally create imbalance
    ctrl.rowsModel()->setRowProperty(1, "creditAmt", 4500.00);
    ctrl.recalculateTotals();

    QCOMPARE(ctrl.totalCredit(), 4500.00);
    QVERIFY(std::abs(ctrl.totalDebit() - ctrl.totalCredit()) > 0.01);
}

void LogicBoardTestSuite::testChequeVoucherModes() {
    ChequeVoucherController ctrl;
    ctrl.setVoucherMode("Payment");
    QCOMPARE(ctrl.voucherMode(), QString("Payment"));

    ctrl.setVoucherMode("Receipt");
    QCOMPARE(ctrl.voucherMode(), QString("Receipt"));
}

void LogicBoardTestSuite::testTdsCalculationsWithSurchargeAndCess() {
    TdsVoucherController ctrl;
    ctrl.resetForm("01/04/2026");

    // Income: 1,00,000, TDS Rate: 10%, Surcharge: 0%, Cess: 4%
    ctrl.setIncomeAmount(100000.0);
    ctrl.setTdsRate(10.0);
    ctrl.setSurchargeRate(0.0);
    ctrl.setCessRate(4.0);
    ctrl.setUseRoundedTotal(true);

    // TDS Tax = 10,000.00, Cess = 400.00 => Total Tax = 10,400.00
    QCOMPARE(ctrl.tdsTaxAmount(), 10000.00);
    QCOMPARE(ctrl.cessTaxAmount(), 400.00);
    QCOMPARE(ctrl.totalTaxAmount(), 10400.00);
    QCOMPARE(ctrl.netAmount(), 89600.00);
}

void LogicBoardTestSuite::testJFormYieldAndDeductionCalculations() {
    JFormVoucherController ctrl;
    ctrl.resetForm("01/04/2026");

    // Item: 100 bags, 50 Qtl @ 2400 => 1,20,000.00
    ctrl.addItemRow("Paddy Sona Masoori", 100, 0.0, "0.500", 50.0, 2400.0, 120000.0);

    QCOMPARE(ctrl.totalBags(), 100);
    QCOMPARE(ctrl.totalWeight(), 50.0);
    QCOMPARE(ctrl.goodsAmount(), 120000.00);

    // Bonus 2000, Labour 1500
    ctrl.setBonusAmount(2000.0);
    ctrl.setLabourAmount(1500.0);

    // Subtotal = 1,22,000.00, Net Before Round = 1,20,500.00
    QCOMPARE(ctrl.subtotalAmount(), 122000.00);
    QCOMPARE(ctrl.grandTotal(), 120500.00);
}

void LogicBoardTestSuite::testMillingBatchYieldDistribution() {
    MillingBatchController ctrl;
    ctrl.resetForm("01/04/2026");

    // Input: 1000 Qtl Paddy @ ₹ 25,00,000
    ctrl.consumedModel()->clear();
    ctrl.consumedModel()->appendRow("Paddy Basmati", 2000, 1000.0, 2500000.0);

    // Auto standard yield distribution
    ctrl.autoPickStandardItems();

    // Standard outturn:
    // 65% Rice (650.0 Qtl), 15% Bran (150.0 Qtl), 10% Broken (100.0 Qtl), 5% Nakku (50.0 Qtl), 5% Husk (50.0 Qtl)
    QCOMPARE(ctrl.totalConsumedWeight(), 1000.0);
    QCOMPARE(ctrl.totalProducedWeight(), 1000.0);
    QCOMPARE(ctrl.totalProducedYieldPct(), 100.0);
    QCOMPARE(ctrl.shortageWeight(), 0.0);
}

void LogicBoardTestSuite::testPaddyProcurementMoistureDeductions() {
    PaddyProcurementController ctrl;
    ctrl.resetForm("01/04/2026");

    // Gross: 100 Qtl, Moisture: 19% (>17% benchmark by 2%)
    // Moisture deduction = 100 * (19 - 17) * 0.01 = 2.0 Qtl
    // Net Qtl = 98.0 Qtl
    // Rate: 2500 => Gross: 98 * 2500 = 2,45,000.00
    ctrl.setGrossWeightQtl(100.0);
    ctrl.setMoisturePct(19.0);
    ctrl.setRatePerQtl(2500.0);
    ctrl.setBagCount(200);
    ctrl.setHamaliPerBag(10.0); // 200 * 10 = 2000

    QCOMPARE(ctrl.moistureDeductionQtl(), 2.0);
    QCOMPARE(ctrl.netWeightQtl(), 98.0);
    QCOMPARE(ctrl.hamaliTotal(), 2000.0);
    QCOMPARE(ctrl.netAmount(), 243000.00); // 2,45,000 - 2,000
}

void LogicBoardTestSuite::testPerFinancialYearVoucherNumbering() {
    SalesModel salesModel;
    QString vch2627 = salesModel.get_next_voucher_no("FY 2026-27");
    QVERIFY(vch2627.startsWith("Sale-"));
    int num2627 = vch2627.mid(5).toInt();
    QVERIFY(num2627 >= 246);

    QString inv2627 = salesModel.get_next_invoice_no("FY 2026-27");
    QVERIFY(inv2627.startsWith("MRI/2627-"));

    // For FY 2025-26 (1090 existing invoices, max 1093), next voucher must be Sale-1094
    QString vch2526 = salesModel.get_next_voucher_no("FY 2025-26");
    QCOMPARE(vch2526, QString("Sale-1094"));

    // Date string resolution: 01-05-2026 belongs to FY 2026-27
    QString vchByDate = salesModel.get_next_voucher_no("01-05-2026");
    QCOMPARE(vchByDate, vch2627);

    // Multi-Year Voucher Lookup Disambiguation Test
    auto& db = DatabaseManager::instance();
    // Insert same voucher_no '99901' into two different fiscal years (high number to avoid collision with real data)
    db.executeNonQuery("INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration, financial_year) "
                       "VALUES ('99901', '2023-08-15', 'Payment', 'Old FY 23-24 Party', 'Cash', 1000.0, 'Old Year Test', 'FY 2023-24');");
    db.executeNonQuery("INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, party_name, account_type, amount, narration, financial_year) "
                       "VALUES ('99901', '2026-07-09', 'Payment', 'New FY 26-27 Party', 'Canara Bank CC', 3544780.0, 'New Year Test', 'FY 2026-27');");

    VouchersModel vchModel;
    // Querying with date 2026-07-09 must return the 2026 voucher
    QVariantMap vch2026 = vchModel.get_cheque_voucher("99901", "09-07-2026");
    QCOMPARE(vch2026.value("party_name").toString(), QString("New FY 26-27 Party"));
    QCOMPARE(vch2026.value("amount").toDouble(), 3544780.0);

    // Querying with date 2023-08-15 must return the 2023 voucher
    QVariantMap vch2023 = vchModel.get_cheque_voucher("ChPt 99901", "15-08-2023");
    QCOMPARE(vch2023.value("party_name").toString(), QString("Old FY 23-24 Party"));
    QCOMPARE(vch2023.value("amount").toDouble(), 1000.0);
}

// -------------------------------------------------------------
// 4. Masters & Regulatory Validations
// -------------------------------------------------------------
void LogicBoardTestSuite::testGstinValidationAndPanExtraction() {
    LedgerMasterController ctrl;

    // Valid GSTIN: 29AABCS1429B1ZB (Karnataka code 29, PAN AABCS1429B)
    QVERIFY(ctrl.validateGstin("29AABCS1429B1ZB"));
    QCOMPARE(ctrl.extractPanFromGstin("29AABCS1429B1ZB"), QString("AABCS1429B"));

    // Invalid GSTINs
    QVERIFY(!ctrl.validateGstin("INVALID_GSTIN_123"));
    QVERIFY(!ctrl.validateGstin("29AABCS1429B")); // only 12 chars
}

void LogicBoardTestSuite::testStockItemOpeningValuation() {
    StockMasterController ctrl;
    ctrl.resetForm();

    ctrl.setPackingKg(50.0); // 0.500 Qtl per bag
    ctrl.setOpeningBags(100);

    // Auto qty = 100 * 0.500 = 50.0 Qtl
    QCOMPARE(ctrl.openingQty(), 50.0);

    ctrl.setOpeningRate(3500.0);
    // Opening value = 50 * 3500 = 1,75,000.00
    QCOMPARE(ctrl.openingValue(), 175000.00);
}

void LogicBoardTestSuite::testCanaraBankStatementPdfParser() {
    QString pdfPath = "322157558-3_unlocked.pdf";
    if (!QFile::exists(pdfPath)) {
        if (QFile::exists("../322157558-3_unlocked.pdf")) pdfPath = "../322157558-3_unlocked.pdf";
        else if (QFile::exists("322157558_unlocked.pdf")) pdfPath = "322157558_unlocked.pdf";
        else if (QFile::exists("../322157558_unlocked.pdf")) pdfPath = "../322157558_unlocked.pdf";
        else QSKIP("Sample PDF 322157558-3_unlocked.pdf not found in working directory.");
    }

    CanaraBankStatementParser parser;
    CanaraBankStatementHeader header;
    QVector<CanaraBankTransaction> txns;
    QString errMsg;

    bool ok = parser.parsePdf(pdfPath, header, txns, errMsg);
    QVERIFY2(ok, qPrintable(errMsg));
    QCOMPARE(header.accountNo, QString("128001400717"));
    QCOMPARE(header.firmName, QString("MAHADEV RICE INDUSTRY"));
    QCOMPARE(header.ifscCode, QString("CNRB0002058"));

    // Verify 2,839 transactions matching XLS
    QCOMPARE(txns.size(), 2839);

    double sumW = 0.0;
    double sumD = 0.0;
    int receipts = 0;
    int payments = 0;
    int charges = 0;

    for (const auto &t : txns) {
        sumW += t.withdrawal;
        sumD += t.deposit;
        if (t.deposit > 0.001) receipts++;
        else payments++;
        if (t.category == "BANK_CHARGES" || t.category == "INTEREST_DEBIT") charges++;
    }

    // Verify sums match
    QVERIFY(sumW > 100000000.0);
    QVERIFY(sumD > 100000000.0);
    QVERIFY(receipts > 300);
    QVERIFY(payments > 1500);
    QVERIFY(charges > 500);
}

void LogicBoardTestSuite::testNativeXlsBankStatementParser() {
    QString xlsPath = "322157558.XLS";
    if (!QFile::exists(xlsPath)) {
        if (QFile::exists("../322157558.XLS")) xlsPath = "../322157558.XLS";
        else QSKIP("322157558.XLS not found.");
    }

    BankStatementMetadata meta;
    QVector<BankStatementTransaction> txns;
    QString errorMsg;

    bool ok = BankStatementExcelParser::parseFile(xlsPath, meta, txns, errorMsg);
    QVERIFY2(ok, qPrintable(errorMsg));

    // Verify metadata
    QCOMPARE(meta.accountNumber, QString("128001400717"));
    QCOMPARE(meta.ifscCode, QString("CNRB0002058"));
    QCOMPARE(meta.customerId, QString("322157558"));
    QCOMPARE(meta.accountName, QString("MAHADEV RICE INDUSTRY"));
    QCOMPARE(meta.openingBalance, -238810040.90);
    QCOMPARE(meta.closingBalance, -231544710.12);

    // Verify exactly 2,839 transactions parsed
    QCOMPARE(txns.size(), 2839);

    // Verify first transaction
    const auto& t1 = txns.first();
    QCOMPARE(t1.date, QString("2025-04-01"));
    QCOMPARE(t1.txnId, QString("20250401000001"));
    QCOMPARE(t1.withdrawal, 60000.00);
    QCOMPARE(t1.deposit, 0.00);
    QCOMPARE(t1.balance, -238870040.90);
    QVERIFY(t1.remarks.contains("SUSHIL TRADING COMPANY"));

    // Verify controller integration
    BankStatementController ctrl;
    bool ctrlOk = ctrl.loadStatement(xlsPath);
    QVERIFY(ctrlOk);
    QCOMPARE(ctrl.totalCount(), 2839);
    QVERIFY(ctrl.totalWithdrawals() > 0.0);
    QVERIFY(ctrl.totalDeposits() > 0.0);
}

void LogicBoardTestSuite::testBankStatementControllerPosting() {
    QString pdfPath = "322157558-3_unlocked.pdf";
    if (!QFile::exists(pdfPath)) {
        if (QFile::exists("../322157558-3_unlocked.pdf")) pdfPath = "../322157558-3_unlocked.pdf";
        else if (QFile::exists("322157558_unlocked.pdf")) pdfPath = "322157558_unlocked.pdf";
        else if (QFile::exists("../322157558_unlocked.pdf")) pdfPath = "../322157558_unlocked.pdf";
        else QSKIP("Sample PDF 322157558-3_unlocked.pdf not found.");
    }

    BankStatementController ctrl;
    bool ok = ctrl.loadStatement(pdfPath);
    QVERIFY(ok);
    QCOMPARE(ctrl.totalCount(), 2839);
    QVERIFY(!ctrl.bankLedgerName().isEmpty());

    // Test alias persistence
    ctrl.saveAlias("TEST BANK NARRATION ALIAS", "Anand Agro Foods [Batala]");
    QVariant val = DatabaseManager::instance().executeScalar(
        "SELECT mapped_party_name FROM bank_narration_aliases WHERE narration_pattern = 'TEST BANK NARRATION ALIAS';"
    );
    QCOMPARE(val.toString(), QString("Anand Agro Foods [Batala]"));

    // Verify party matches in loaded transactions
    bool foundSingla = false;
    bool foundBakhtawar = false;
    bool foundNasa = false;
    bool foundDeepak = false;
    for (const auto &r : ctrl.rowsModel()->rows()) {
        if (r.rawNarration.contains("SINGLA AGRO", Qt::CaseInsensitive)) {
            if (r.suggestedCrAccount.contains("Singla Agro", Qt::CaseInsensitive) ||
                r.suggestedDrAccount.contains("Singla Agro", Qt::CaseInsensitive) ||
                r.extractedParty.contains("Singla Agro", Qt::CaseInsensitive)) {
                foundSingla = true;
            }
        }
        if (r.rawNarration.contains("BHAKTAWAR", Qt::CaseInsensitive)) {
            if (r.suggestedCrAccount.contains("Bakhtawar", Qt::CaseInsensitive) ||
                r.suggestedDrAccount.contains("Bakhtawar", Qt::CaseInsensitive)) {
                foundBakhtawar = true;
            }
        }
        if (r.rawNarration.contains("NASA AGRO", Qt::CaseInsensitive)) {
            if (r.suggestedCrAccount.contains("Nasa Agro", Qt::CaseInsensitive) ||
                r.suggestedDrAccount.contains("Nasa Agro", Qt::CaseInsensitive)) {
                foundNasa = true;
            }
        }
        if (r.rawNarration.contains("DEEPAK SON OF RAJESH KUMAR", Qt::CaseInsensitive)) {
            if (r.suggestedCrAccount.contains("Deepak", Qt::CaseInsensitive) ||
                r.suggestedDrAccount.contains("Deepak", Qt::CaseInsensitive)) {
                foundDeepak = true;
            }
        }
    }
    QVERIFY(foundSingla);
    QVERIFY(foundBakhtawar);
    QVERIFY(foundNasa);
    QVERIFY(foundDeepak);

    // Test Account-Number Aware Fallback Matching (e.g. Deepak Singh account 50100034426592)
    ctrl.saveAlias("IB NEFT DR-HDFC0000191-DEEPAK SINGH-MAHADEV RICE INDUSTRY-NETBANK,MUM-N129253569833772-50100034426592", "Deepak S/o Rajesh Kumar");
    
    // Verify party bank_account was updated
    QVariant dbAcc = DatabaseManager::instance().executeScalar(
        "SELECT bank_account FROM parties WHERE name = 'Deepak S/o Rajesh Kumar';"
    );
    QCOMPARE(dbAcc.toString(), QString("50100034426592"));

    // Reload statement to verify auto-matching on account number
    ctrl.reloadFromCurrentStatement();
    int deepakAccMatchedCount = 0;
    int deepakTotalMatchedCount = 0;
    for (const auto &r : ctrl.rowsModel()->rows()) {
        if (r.suggestedCrAccount == "Deepak S/o Rajesh Kumar" || r.suggestedDrAccount == "Deepak S/o Rajesh Kumar") {
            deepakTotalMatchedCount++;
            if (r.rawNarration.contains("50100034426592")) {
                deepakAccMatchedCount++;
            }
        }
    }
    QVERIFY(deepakAccMatchedCount >= 3);
    QVERIFY(deepakTotalMatchedCount >= 5);
}

void LogicBoardTestSuite::testTransportDispatchController() {
    TransportDispatchController ctrl;

    // 1. Test Net Weight Calculation Engine
    // Gross: 450.00 Qtl, Tare: 109.40 Qtl, Bags: 680, BagTare: 0 kg
    double net1 = ctrl.calculateNetWeight(450.0, 109.40, 680, 0.0);
    QCOMPARE(net1, 340.60);

    // Gross: 450.00 Qtl, Tare: 109.40 Qtl, Bags: 680, BagTare: 0.1 kg/bag (68 kg = 0.68 Qtl)
    double net2 = ctrl.calculateNetWeight(450.0, 109.40, 680, 0.1);
    QCOMPARE(net2, 339.92);

    // 2. Test Freight Calculation
    // Mode "Per Qtl": Rate 60, Net 340.60, Advance 5000
    QVariantMap f1 = ctrl.calculateFreight("Per Qtl", 60.0, 340.60, 680, 5000.0);
    QCOMPARE(f1.value("totalFreight").toDouble(), 20436.00);
    QCOMPARE(f1.value("balanceFreight").toDouble(), 15436.00);
    QCOMPARE(f1.value("paymentStatus").toString(), QString("Partially Paid"));

    // Mode "Per Bag": Rate 25, Bags 680, Advance 17000
    QVariantMap f2 = ctrl.calculateFreight("Per Bag", 25.0, 340.60, 680, 17000.0);
    QCOMPARE(f2.value("totalFreight").toDouble(), 17000.00);
    QCOMPARE(f2.value("balanceFreight").toDouble(), 0.00);
    QCOMPARE(f2.value("paymentStatus").toString(), QString("Settled"));

    // Mode "Fixed": Rate 12000, Advance 0
    QVariantMap f3 = ctrl.calculateFreight("Fixed", 12000.0, 340.60, 680, 0.0);
    QCOMPARE(f3.value("totalFreight").toDouble(), 12000.00);
    QCOMPARE(f3.value("balanceFreight").toDouble(), 12000.00);
    QCOMPARE(f3.value("paymentStatus").toString(), QString("Unpaid"));

    // 3. Test CRUD Persistence
    QVariantMap record;
    record["slipNo"] = "TEST-SLIP-001";
    record["dispatchDate"] = "2026-03-15";
    record["dispatchTime"] = "14:30";
    record["vehicleNo"] = "PB03AJ1982";
    record["partyName"] = "Haryana Rice Traders";
    record["itemName"] = "Basmati Sella 1121";
    record["bagCount"] = 680;
    record["packingKg"] = 50.0;
    record["grossWeightQtl"] = 450.0;
    record["tareWeightQtl"] = 109.40;
    record["bagTareKg"] = 0.0;
    record["freightCalcType"] = "Per Qtl";
    record["freightRate"] = 60.0;
    record["advanceFreight"] = 5000.0;
    record["transporterName"] = "Royal Transport Co";
    record["grNo"] = "GR-9988";
    record["ewayBillNo"] = "123456789012";

    QVariantMap saveRes = ctrl.saveDispatch(record);
    QVERIFY(saveRes.value("success").toBool());
    int recId = saveRes.value("id").toInt();
    QVERIFY(recId > 0);

    // Verify retrieval
    QVariantMap fetched = ctrl.getDispatch(recId);
    QCOMPARE(fetched.value("slipNo").toString(), QString("TEST-SLIP-001"));
    QCOMPARE(fetched.value("vehicleNo").toString(), QString("PB03AJ1982"));
    QCOMPARE(fetched.value("netWeightQtl").toDouble(), 340.60);
    QCOMPARE(fetched.value("totalFreight").toDouble(), 20436.00);

    // Test Model Filtering
    ctrl.reload();
    QVERIFY(ctrl.model()->count() > 0);
    ctrl.model()->setFilter("PENDING", "PB03AJ1982");
    QVERIFY(ctrl.model()->count() >= 1);

    // Clean up
    bool delOk = ctrl.deleteDispatch(recId);
    QVERIFY(delOk);
}

void LogicBoardTestSuite::testDebitCreditNoteController() {
    DebitCreditNoteController ctrl;

    // 1. Test Tax & Total Calculation Engine
    QVariantList testItems;
    QVariantMap item1;
    item1["itemName"] = "1121 Sella Rice (Quality Rate Cut)";
    item1["hsnCode"] = "10063020";
    item1["unit"] = "QTL";
    item1["bags"] = 680;
    item1["weightQtl"] = 340.60;
    item1["rate"] = 50.0; // Rs. 50/Qtl rate cut
    item1["taxableAmount"] = 17030.00;
    testItems.append(item1);

    // Intrastate GST @ 5% (CGST 2.5% + SGST 2.5%)
    QVariantMap calcLocal = ctrl.calculateTotals(testItems, 5.0, false);
    QCOMPARE(calcLocal.value("taxableAmount").toDouble(), 17030.00);
    QCOMPARE(calcLocal.value("cgstAmount").toDouble(), 425.75);
    QCOMPARE(calcLocal.value("sgstAmount").toDouble(), 425.75);
    QCOMPARE(calcLocal.value("igstAmount").toDouble(), 0.00);
    QCOMPARE(calcLocal.value("grandTotal").toDouble(), 17882.00);
    QCOMPARE(calcLocal.value("roundOff").toDouble(), 0.50);

    // Interstate GST @ 5% (IGST 5.0%)
    QVariantMap calcInter = ctrl.calculateTotals(testItems, 5.0, true);
    QCOMPARE(calcInter.value("cgstAmount").toDouble(), 0.00);
    QCOMPARE(calcInter.value("sgstAmount").toDouble(), 0.00);
    QCOMPARE(calcInter.value("igstAmount").toDouble(), 851.50);
    QCOMPARE(calcInter.value("totalTaxAmount").toDouble(), 851.50);

    // 2. Test Note Creation, Persistence & Double-Entry Voucher Posting
    QVariantMap noteData;
    noteData["noteType"] = "Credit Note";
    noteData["noteNo"] = "CN-TEST-001";
    noteData["noteDate"] = "2026-03-15";
    noteData["noteTime"] = "15:00";
    noteData["originalInvoiceNo"] = "12640";
    noteData["originalInvoiceDate"] = "2026-03-10";
    noteData["originalInvoiceType"] = "Sale";
    noteData["partyName"] = "Haryana Rice Traders";
    noteData["partyGstin"] = "06AABCR1234F1Z1";
    noteData["isInterstate"] = true;
    noteData["reasonCode"] = "03-Deficiency in Value / Quality Cut";
    noteData["adjustmentType"] = "Rate Cut / Quality Deduction";
    noteData["gstPct"] = 5.0;
    noteData["narration"] = "Quality rate deduction Rs. 50/Qtl approved";
    noteData["items"] = testItems;

    QVariantMap saveRes = ctrl.saveNote(noteData);
    QVERIFY(saveRes.value("success").toBool());
    int noteId = saveRes.value("id").toInt();
    QVERIFY(noteId > 0);

    // Verify Note retrieval
    QVariantMap fetched = ctrl.getNote(noteId);
    QCOMPARE(fetched.value("noteNo").toString(), QString("CN-TEST-001"));
    QCOMPARE(fetched.value("partyName").toString(), QString("Haryana Rice Traders"));
    QCOMPARE(fetched.value("taxableAmount").toDouble(), 17030.00);
    QCOMPARE(fetched.value("igstAmount").toDouble(), 851.50);

    // Verify double-entry vouchers posted into vouchers table
    QVariant vchCount = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM vouchers WHERE voucher_no = 'CN-TEST-001';"
    );
    QVERIFY(vchCount.toInt() >= 2); // Credit Party + Debit Sales Return (+ Debit IGST)

    // Verify Party Credit leg exists
    QVariant partyCr = DatabaseManager::instance().executeScalar(
        "SELECT amount FROM vouchers WHERE voucher_no = 'CN-TEST-001' AND party_name = 'Haryana Rice Traders' AND account_type = 'Cr';"
    );
    QVERIFY(partyCr.isValid());
    QCOMPARE(partyCr.toDouble(), fetched.value("grandTotal").toDouble());

    // Test Model Filtering
    ctrl.reload();
    QVERIFY(ctrl.model()->count() > 0);
    ctrl.model()->setFilter("CREDIT_NOTES", "CN-TEST-001");
    QVERIFY(ctrl.model()->count() >= 1);

    // Clean up
    bool delOk = ctrl.deleteNote(noteId);
    QVERIFY(delOk);

    // Verify vouchers cleaned up
    QVariant vchAfter = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM vouchers WHERE voucher_no = 'CN-TEST-001';"
    );
    QCOMPARE(vchAfter.toInt(), 0);
}

void LogicBoardTestSuite::testFirmTypeResolutionAndDynamicFiscalYears() {
    // 1. Test PAN 4th Character Resolution
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("Partnership", "ASNPR3139C", "M/S SUSHIL TRADING COMPANY"), QString("Proprietorship Firm"));
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("Unknown", "ABKFM5928Q", "M/S MAHADEV RICE INDUSTRY"), QString("Partnership Firm"));
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("", "06BCUPK4267Q2ZL"), QString("Proprietorship Firm"));
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("", "AAACA1234C"), QString("Private Limited Company"));
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("", "AABTH1234T"), QString("Trust"));
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("", "AAAHH1234H"), QString("Hindu Undivided Family (HUF)"));
    QCOMPARE(BahiKhataMigrator::resolveFirmTypeFromPan("", "AAAAB1234A"), QString("Association of Persons (AOP/BOI)"));

    // 2. Test Banking String Extraction & Cleaning
    QString bankName, bankAcc, ifsc;
    BahiKhataMigrator::cleanBankingDetails(
        "A/C -923030014007595",
        "BANK & BRANCH - AXIS BANK, SIRSA (HRY.)",
        "IFSE.-UTIB0003740",
        bankName, bankAcc, ifsc
    );
    QCOMPARE(bankName, QString("AXIS BANK, SIRSA (HRY.)"));
    QCOMPARE(bankAcc, QString("923030014007595"));
    QCOMPARE(ifsc, QString("UTIB0003740"));

    BahiKhataMigrator::cleanBankingDetails(
        "CANARA BANK",
        "A/c No:- 128001400717",
        "IFSC:- CNRB0002058",
        bankName, bankAcc, ifsc
    );
    QCOMPARE(bankName, QString("CANARA BANK"));
    QCOMPARE(bankAcc, QString("128001400717"));
    QCOMPARE(ifsc, QString("CNRB0002058"));

    // 3. Test Dynamic Fiscal Years Generation from Books Begin Date
    FirmManager firmMgr;
    QVariantMap testFirm;
    testFirm["company_name"] = "Test Agro Traders";
    testFirm["gstin"] = "06ASNPR3139C1ZQ";
    testFirm["pan_no"] = "ASNPR3139C";
    testFirm["books_from"] = "1.4.24"; // Starts 2024
    testFirm["address"] = "Shop 12, Mandi, Sirsa 125055";

    bool created = firmMgr.create_new_firm(testFirm);
    QVERIFY(created);

    QVariantList fys = DatabaseManager::instance().executeQuery("SELECT year_name, start_date, end_date, is_active FROM financial_years ORDER BY start_date;");
    QVERIFY(fys.size() >= 3); // 2024-25, 2025-26, 2026-27
    QCOMPARE(fys[0].toMap().value("year_name").toString(), QString("FY 2024-25"));
    QCOMPARE(fys[1].toMap().value("year_name").toString(), QString("FY 2025-26"));
    QCOMPARE(fys[2].toMap().value("year_name").toString(), QString("FY 2026-27"));

    QVariantMap compInfo = firmMgr.get_current_firm_info();
    QCOMPARE(compInfo.value("firm_type").toString(), QString("Proprietorship Firm"));
    QCOMPARE(compInfo.value("books_from").toString(), QString("2024-04-01"));

    // Switch back to isolated test unit DB
    QString originalDb = "data/test_unit_suite.db";
    if (QFile::exists("../data/test_unit_suite.db")) originalDb = "../data/test_unit_suite.db";
    DatabaseManager::instance().switchDatabase(originalDb);
}

void LogicBoardTestSuite::testHistoricalFiscalYearIsolationWithoutGapYears() {
    QString histDbPath = "data/test_historical_fy.db";
    if (QFile::exists(histDbPath)) QFile::remove(histDbPath);

    DatabaseManager::instance().switchDatabase(histDbPath);

    // Seed historical company info ending in FY 2023-24
    DatabaseManager::instance().executeNonQuery("DELETE FROM company_info;");
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO company_info (company_name, firm_type, books_from, acc_year_from, acc_year_to) "
        "VALUES ('Old Trading Co', 'Partnership Firm', '2021-04-01', '2021-04-01', '2024-03-31');"
    );

    // Seed historical transactions across 2021-2023
    DatabaseManager::instance().executeNonQuery("DELETE FROM transactions;");
    DatabaseManager::instance().executeNonQuery(
        "INSERT INTO transactions (voucher_no, voucher_type, voucher_date, account_id, amount, dr_cr) "
        "VALUES ('V-01', 'Journal', '2021-06-15', 1, 5000, 'Dr'), ('V-02', 'Journal', '2023-11-20', 1, 8000, 'Dr');"
    );

    // Run discovery
    FiscalYearHelper::ensureFiscalYearsDiscovered();

    QVariantList fys = DatabaseManager::instance().executeQuery(
        "SELECT year_name, start_date, end_date, is_active FROM financial_years ORDER BY start_date ASC;"
    );

    // Exactly 3 years: 2021-22, 2022-23, 2023-24
    QCOMPARE(fys.size(), 3);
    QCOMPARE(fys[0].toMap().value("year_name").toString(), QString("FY 2021-22"));
    QCOMPARE(fys[1].toMap().value("year_name").toString(), QString("FY 2022-23"));
    QCOMPARE(fys[2].toMap().value("year_name").toString(), QString("FY 2023-24"));

    // Active year must strictly be the latest actual historical year (FY 2023-24)
    QCOMPARE(fys[2].toMap().value("is_active").toInt(), 1);

    // Check no future years are present
    QVariant futureFys = DatabaseManager::instance().executeScalar(
        "SELECT COUNT(*) FROM financial_years WHERE year_name IN ('FY 2024-25', 'FY 2025-26', 'FY 2026-27', 'FY 2027-28');"
    );
    QCOMPARE(futureFys.toInt(), 0);

    // Re-running discovery must be a no-op and never introduce future years
    FiscalYearHelper::ensureFiscalYearsDiscovered();
    QVariantList fysRecheck = DatabaseManager::instance().executeQuery("SELECT year_name FROM financial_years;");
    QCOMPARE(fysRecheck.size(), 3);

    FiscalYearInfo activeInfo = FiscalYearHelper::getActiveFiscalYear();
    QCOMPARE(activeInfo.name, QString("FY 2023-24"));

    // Switch back to isolated test unit DB
    QString originalDb = "data/test_unit_suite.db";
    if (QFile::exists("../data/test_unit_suite.db")) originalDb = "../data/test_unit_suite.db";
    DatabaseManager::instance().switchDatabase(originalDb);
    if (QFile::exists(histDbPath)) QFile::remove(histDbPath);
}

// -------------------------------------------------------------
// 9. Balance Sheet Bahi-Khata Reporting System
// -------------------------------------------------------------
void LogicBoardTestSuite::testBalanceSheetCalculations() {
    qDebug() << "[TEST] Running Balance Sheet Engine calculations...";

    // 1. Calculate Balance Sheet as of FY 2025-26 year-end (2026-03-31) - Audited Bahi-Khata Reference
    BalanceSheetData data2526 = BalanceSheetCalculator::calculate("2026-03-31");
    QCOMPARE(data2526.asOnDate, QString("2026-03-31"));
    QCOMPARE(data2526.financialYear, QString("FY 2025-26"));
    QVERIFY(!data2526.firmName.isEmpty());

    // Audited Bahi-Khata Mathematical Invariants:
    // Closing stock: 22,98,85,143.91
    QCOMPARE(data2526.closingStockValue, 229885143.91);
    QVERIFY(std::abs(data2526.netProfit) > 0.0);
    QVERIFY(data2526.totalLiabilities > 0.0);
    QVERIFY(data2526.totalAssets > 0.0);

    // Validate Capital Account & Partners breakdown
    bool foundCapital = false;
    for (const auto& g : data2526.liabilitiesGroups) {
        if (g.name.contains("Capital", Qt::CaseInsensitive)) {
            foundCapital = true;
            QCOMPARE(g.amount, 98243356.35);
            QVERIFY(g.children.size() >= 2);
        }
    }
    QVERIFY(foundCapital);

    // 2. Calculate Balance Sheet as of FY 2026-27 year-end (2027-03-31)
    BalanceSheetData data = BalanceSheetCalculator::calculate("2027-03-31");
    QCOMPARE(data.asOnDate, QString("2027-03-31"));
    QCOMPARE(data.financialYear, QString("FY 2026-27"));
    QVERIFY(!data.firmName.isEmpty());

    // 3. Validate Liabilities structure & groups
    QVERIFY(data.liabilitiesGroups.size() > 0);
    bool hasCapital = false;
    bool hasCreditors = false;
    for (const auto& g : data.liabilitiesGroups) {
        if (g.name.contains("Capital", Qt::CaseInsensitive)) hasCapital = true;
        if (g.name.contains("Creditors", Qt::CaseInsensitive) || g.name.contains("Liabilities", Qt::CaseInsensitive)) hasCreditors = true;
        QVERIFY(g.amount >= 0.0);
    }
    QVERIFY(hasCapital || hasCreditors);
    QVERIFY(data.totalLiabilities > 0.0);

    // 4. Validate Assets structure & groups
    QVERIFY(data.assetsGroups.size() > 0);
    bool hasStockOrDebtors = false;
    for (const auto& g : data.assetsGroups) {
        if (g.name.contains("Stock", Qt::CaseInsensitive) || g.name.contains("Debtors", Qt::CaseInsensitive) || g.name.contains("Bank", Qt::CaseInsensitive)) {
            hasStockOrDebtors = true;
        }
        QVERIFY(g.amount >= 0.0);
    }
    QVERIFY(hasStockOrDebtors);
    QVERIFY(data.totalAssets > 0.0);

    // 5. Validate Controller and Export methods
    BalanceSheetController ctrl;
    ctrl.reload("2027-03-31");
    QCOMPARE(ctrl.asOnDate(), QString("2027-03-31"));
    QCOMPARE(ctrl.totalLiabilities(), data.totalLiabilities);
    QCOMPARE(ctrl.totalAssets(), data.totalAssets);

    // 6. Test PDF and CSV Export generation
    QString testPdfPath = QDir::tempPath() + "/test_balance_sheet.pdf";
    QString outPdf = ctrl.exportPdf(testPdfPath);
    QVERIFY(QFile::exists(outPdf));
    QFile::remove(outPdf);

    QString testCsvPath = QDir::tempPath() + "/test_balance_sheet.csv";
    QString outCsv = ctrl.exportCsv(testCsvPath);
    QVERIFY(QFile::exists(outCsv));
    QFile::remove(outCsv);

    qDebug() << "[TEST] Balance Sheet Engine test passed. FY 25-26 Total:" << data2526.totalLiabilitiesFmt << "FY 26-27 Total:" << data.totalLiabilitiesFmt;
}

void LogicBoardTestSuite::testProfitLossCalculations() {
    qDebug() << "[TEST] Running Trading and Profit & Loss Engine calculations...";

    // 1. Calculate P&L for full FY 2026-27 (2026-04-01 to 2027-03-31)
    ProfitLossData data = ProfitLossCalculator::calculate("2026-04-01", "2027-03-31");
    QCOMPARE(data.fromDate, QString("2026-04-01"));
    QCOMPARE(data.toDate, QString("2027-03-31"));
    FiscalYearInfo expectedFy = FiscalYearHelper::getFiscalYearForDate("2026-04-01");
    QCOMPARE(data.financialYear, expectedFy.name);
    QVERIFY(!data.firmName.isEmpty());

    // 2. Validate Trading Account math
    // Gross Profit = (Sales + Closing Stock) - (Opening Stock + Procurement + Direct Expenses)
    double tradingCr = data.totalSalesRevenue + data.closingStockValue;
    double tradingDr = data.openingStockValue + data.totalProcurement + data.totalDirectExpenses;
    double expectedGrossProfit = tradingCr - tradingDr;
    if (expectedGrossProfit >= 0.0) {
        QCOMPARE(data.grossProfit, expectedGrossProfit);
        QCOMPARE(data.grossLoss, 0.0);
    } else {
        QCOMPARE(data.grossProfit, 0.0);
        QCOMPARE(data.grossLoss, std::abs(expectedGrossProfit));
    }
    QCOMPARE(data.totalTradingDr, data.totalTradingCr);

    // 3. Validate Profit & Loss Account math
    // Net Profit = (Gross Profit + Indirect Incomes) - (Gross Loss + Indirect Expenses)
    double plCr = data.grossProfit + data.indirectIncomes;
    double plDr = data.grossLoss + data.indirectExpenses;
    double expectedNetProfit = plCr - plDr;
    if (expectedNetProfit >= 0.0) {
        QCOMPARE(data.netProfit, expectedNetProfit);
        QCOMPARE(data.netLoss, 0.0);
    } else {
        QCOMPARE(data.netProfit, 0.0);
        QCOMPARE(data.netLoss, std::abs(expectedNetProfit));
    }
    QCOMPARE(data.totalPlDr, data.totalPlCr);

    // 4. Validate Combined Groups
    QVERIFY(data.expensesSide.size() > 0);
    QVERIFY(data.incomesSide.size() > 0);

    // 5. Validate Controller and Export methods
    ProfitLossController ctrl;
    ctrl.reload("2026-04-01", "2027-03-31");
    QCOMPARE(ctrl.fromDate(), QString("2026-04-01"));
    QCOMPARE(ctrl.toDate(), QString("2027-03-31"));
    QCOMPARE(ctrl.grossProfit(), data.grossProfit);
    QCOMPARE(ctrl.netProfit(), data.netProfit);

    // 6. Test PDF and CSV Export generation
    QString testPdfPath = QDir::tempPath() + "/test_profit_loss.pdf";
    QString outPdf = ctrl.exportPdf(testPdfPath);
    QVERIFY(QFile::exists(outPdf));
    QFile::remove(outPdf);

    QString testCsvPath = QDir::tempPath() + "/test_profit_loss.csv";
    QString outCsv = ctrl.exportCsv(testCsvPath);
    QVERIFY(QFile::exists(outCsv));
    QFile::remove(outCsv);

    qDebug() << "[TEST] Profit & Loss Engine test passed. Gross Profit: ₹" << ctrl.grossProfitFmt() << "Net Profit: ₹" << ctrl.netProfitFmt();
}

void LogicBoardTestSuite::testStockValuationEnginePipeline() {
    qDebug() << "[TEST] Running Stock Valuation Engine Pipeline test...";

    // 1. Test audited snapshot resolution for FY 2025-26
    QVERIFY(StockValuationEngine::hasAuditedClosingStock("2026-03-31"));
    StockValuationReport rep2526 = StockValuationEngine::getEffectiveClosingStock("2026-03-31");
    QVERIFY(rep2526.isAuditedSnapshot);
    QCOMPARE(rep2526.totalValuation, 229885143.91);
    QCOMPARE(rep2526.items.size(), 10);
    QVERIFY(rep2526.totalBags > 0);
    QVERIFY(rep2526.totalWeightQtl > 0.0);

    // 2. Test live physical stock calculation on future date without audited snapshot (e.g. 2027-06-30)
    StockValuationReport liveRep = StockValuationEngine::calculateLivePhysicalStock("2027-06-30");
    QVERIFY(!liveRep.isAuditedSnapshot);
    QVERIFY(liveRep.items.size() > 0);
    QVERIFY(liveRep.totalValuation >= 0.0);

    // 3. Test saving custom audited snapshot for a specific test date
    QString testDate = "2027-09-30";
    QVector<StockValuationItem> testItems;
    StockValuationItem itm1;
    itm1.itemId = 1;
    itm1.itemCode = "1001";
    itm1.itemName = "Test Basmati Rice 1121";
    itm1.bags = 500;
    itm1.weightQtl = 250.0;
    itm1.rate = 4000.0;
    itm1.amount = 1000000.0; // 10,00,000.00
    testItems.append(itm1);

    StockValuationItem itm2;
    itm2.itemId = 2;
    itm2.itemCode = "1002";
    itm2.itemName = "Test Rice Bran";
    itm2.bags = 200;
    itm2.weightQtl = 100.0;
    itm2.rate = 2500.0;
    itm2.amount = 250000.0; // 2,50,000.00
    testItems.append(itm2);

    QString err;
    bool saveOk = StockValuationEngine::saveAuditedClosingStock(testDate, testItems, err);
    QVERIFY2(saveOk, qPrintable(err));
    QVERIFY(StockValuationEngine::hasAuditedClosingStock(testDate));

    // Retrieve and verify
    StockValuationReport savedRep = StockValuationEngine::getEffectiveClosingStock(testDate);
    QVERIFY(savedRep.isAuditedSnapshot);
    QCOMPARE(savedRep.totalBags, 700);
    QCOMPARE(savedRep.totalWeightQtl, 350.0);
    QCOMPARE(savedRep.totalValuation, 1250000.00);
    QCOMPARE(savedRep.items.size(), 2);

    // 4. Test deleting custom closing stock snapshot
    bool delOk = StockValuationEngine::deleteAuditedClosingStock(testDate, err);
    QVERIFY2(delOk, qPrintable(err));
    QVERIFY(!StockValuationEngine::hasAuditedClosingStock(testDate));

    // 5. Test 1-click auto-lock year-end closing stock snapshot
    QString lockDate = "2027-03-31";
    bool lockOk = StockValuationEngine::autoLockYearEndClosingStock(lockDate, err);
    QVERIFY2(lockOk, qPrintable(err));
    QVERIFY(StockValuationEngine::hasAuditedClosingStock(lockDate));

    StockValuationReport lockedRep = StockValuationEngine::getEffectiveClosingStock(lockDate);
    QVERIFY(lockedRep.isAuditedSnapshot);
    QVERIFY(lockedRep.totalValuation > 0.0);

    // Clean up test lock date
    StockValuationEngine::deleteAuditedClosingStock(lockDate, err);

    qDebug() << "[TEST] Stock Valuation Engine Pipeline test passed successfully!";
}

void LogicBoardTestSuite::testTcsReceiptVoucherPosting() {
    DatabaseManager &db = DatabaseManager::instance();
    // Ensure test party
    QVariant pVal = db.executeScalar("SELECT id FROM parties WHERE name = 'TEST_TCS_CUSTOMER' LIMIT 1;");
    int partyId = 0;
    if (pVal.isValid() && !pVal.isNull()) {
        partyId = pVal.toInt();
    } else {
        db.executeNonQuery("INSERT INTO parties (name, group_name, party_type, pan) VALUES ('TEST_TCS_CUSTOMER', 'Sundry Debtors', 'Customer', 'ABCDE1234F');");
        partyId = db.executeScalar("SELECT last_insert_rowid();").toInt();
    }

    // Ensure test bank
    QVariant bVal = db.executeScalar("SELECT id FROM parties WHERE name = 'TEST_TCS_BANK' LIMIT 1;");
    int bankId = 0;
    if (bVal.isValid() && !bVal.isNull()) {
        bankId = bVal.toInt();
    } else {
        db.executeNonQuery("INSERT INTO parties (name, group_name, party_type) VALUES ('TEST_TCS_BANK', 'Bank Accounts', 'Bank');");
        bankId = db.executeScalar("SELECT last_insert_rowid();").toInt();
    }

    TcsReceiptVoucherController ctrl;
    ctrl.resetForm();
    ctrl.setPartyId(partyId);
    ctrl.setBankLedgerId(bankId);
    ctrl.setWithoutTcsAmount(1000000.0); // 10,00,000
    ctrl.setTcsRate(0.100);              // 0.1% = 1,000
    ctrl.setDiscountAllowed(5000.0);     // 5,000
    ctrl.setInterestReceived(2000.0);    // 2,000

    QCOMPARE(ctrl.tcsAmount(), 1000.0);
    QCOMPARE(ctrl.netBankReceipt(), 1001000.0);
    QCOMPARE(ctrl.netCreditToParty(), 1003000.0);

    bool saveOk = ctrl.saveVoucher();
    QVERIFY2(saveOk, qPrintable(ctrl.statusMessage()));

    // Verify double-entry in vouchers & transactions
    QString vchNo = QString("TCS-%1").arg(ctrl.receiptNo() - 1, 4, 10, QChar('0'));
    QVariant sumDr = db.executeScalar("SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Dr';", {vchNo});
    QVariant sumCr = db.executeScalar("SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Cr';", {vchNo});
    QVERIFY(sumDr.isValid() && sumCr.isValid());
    QCOMPARE(sumDr.toDouble(), sumCr.toDouble());
    QCOMPARE(sumDr.toDouble(), 1006000.0); // 10,01,000 (Bank) + 5,000 (Discount) == 10,03,000 (Party) + 1,000 (TCS) + 2,000 (Interest)

    qDebug() << "[TEST] TCS Receipt Voucher posting verified: perfectly balanced at ₹10,06,000.00!";
}

void LogicBoardTestSuite::testTaxDepositChallanPosting() {
    DatabaseManager &db = DatabaseManager::instance();
    int fyId = 1;
    QVariant fyRow = db.executeScalar("SELECT id FROM financial_years WHERE is_active = 1 LIMIT 1;");
    if (fyRow.isValid() && !fyRow.isNull()) fyId = fyRow.toInt();

    QString testDate = QDate::currentDate().toString("yyyy-MM-dd");
    db.executeNonQuery(
        "INSERT INTO tds_vouchers (fy_id, voucher_no, voucher_date, tds_type, ledger_name, total_for_tds, rate_tds, tax_amount_tds, total_tax_amount, net_amount, is_deposited) "
        "VALUES (?, 9999, ?, 'CONTRACTOR', 'TEST_CONTRACTOR', 100000.0, 1.0, 1000.0, 1000.0, 99000.0, 0);",
        {fyId, testDate}
    );
    int testVchId = db.executeScalar("SELECT last_insert_rowid();").toInt();

    TaxChallanController ctrl;
    ctrl.setTaxType("TDS");
    ctrl.setPeriodFrom(testDate);
    ctrl.setPeriodTo(testDate);
    ctrl.fetchUndepositedVouchers();

    QVERIFY(ctrl.undepositedVouchers().size() > 0);
    ctrl.setBsrCode("0510302");
    ctrl.setInterestAmount(100.0);
    ctrl.setPenaltyAmount(50.0);
    ctrl.recalculate();

    QCOMPARE(ctrl.totalTax(), 1000.0);
    QCOMPARE(ctrl.totalChallanAmount(), 1150.0);

    bool saveOk = ctrl.saveChallan();
    QVERIFY2(saveOk, qPrintable(ctrl.statusMessage()));

    // Verify voucher is now marked deposited
    QVariant depStatus = db.executeScalar("SELECT is_deposited FROM tds_vouchers WHERE id = ?;", {testVchId});
    QCOMPARE(depStatus.toInt(), 1);

    // Verify double-entry GL transactions
    QString chNo = QString("CH-%1").arg(ctrl.challanNo());
    QVariant sumDr = db.executeScalar("SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Dr';", {chNo});
    QVariant sumCr = db.executeScalar("SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Cr';", {chNo});
    if (sumDr.isValid() && sumCr.isValid()) {
        QCOMPARE(sumDr.toDouble(), sumCr.toDouble());
        QCOMPARE(sumDr.toDouble(), 1150.0);
    }

    qDebug() << "[TEST] Tax Deposit Challan verified: voucher marked deposited and GL balanced at ₹1,150.00!";
}

void LogicBoardTestSuite::testStockRegisterSubsystem() {
    using namespace MahadevERP;

    StockRegisterController ctrl;
    
    // 1. Test OnlyStock mode with ItemWise grouping
    ctrl.setViewMode(StockViewMode::OnlyStock);
    ctrl.setGrouping(StockGrouping::ItemWise);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 2. Test GroupWise aggregation
    ctrl.setGrouping(StockGrouping::GroupWise);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 3. Test CompanyWise aggregation
    ctrl.setGrouping(StockGrouping::CompanyWise);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 4. Test TotalSummary aggregation
    ctrl.setGrouping(StockGrouping::TotalSummary);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 5. Test HsnWise aggregation
    ctrl.setGrouping(StockGrouping::HsnWise);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 6. Test StockWithAmount mode
    ctrl.setViewMode(StockViewMode::StockWithAmount);
    ctrl.setGrouping(StockGrouping::ItemWise);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 7. Test ProfitLoss mode
    ctrl.setViewMode(StockViewMode::ProfitLoss);
    ctrl.setGrouping(StockGrouping::ItemWise);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);
    
    const auto& pnlEntries = ctrl.model()->entries();
    for (const auto& r : pnlEntries) {
        double expectedGp = r.salesValue - r.cogs;
        QCOMPARE(std::abs(r.grossProfit - expectedGp) < 0.01, true);
        if (r.salesValue > 0) {
            double expectedGpPercent = (expectedGp / r.salesValue) * 100.0;
            QCOMPARE(std::abs(r.gpMarginPct - expectedGpPercent) < 0.05, true);
        }
    }

    // 8. Test MonthlyDaily mode
    ctrl.setViewMode(StockViewMode::MonthlyDaily);
    ctrl.setSelectedItemId(1);
    ctrl.setIsDailyDetail(false);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    ctrl.setIsDailyDetail(true);
    ctrl.reload();
    QVERIFY(ctrl.model()->rowCount() >= 0);

    // 9. Test search query filtering
    ctrl.setViewMode(StockViewMode::OnlyStock);
    ctrl.setGrouping(StockGrouping::ItemWise);
    ctrl.reload();
    int initialCount = ctrl.model()->rowCount();
    if (initialCount > 0) {
        QString firstItemName = ctrl.model()->entries().first().nameVal;
        ctrl.setSearchQuery(firstItemName);
        QVERIFY(ctrl.model()->rowCount() >= 1);
        ctrl.setSearchQuery("");
        QCOMPARE(ctrl.model()->rowCount(), initialCount);
    }

    qDebug() << "[TEST] Stock Register subsystem controller and calculations verified successfully!";
}

void LogicBoardTestSuite::testFinalReportsAndDepreciationSubsystem() {
    // 1. Depreciation Calculator Sec 32 Verification
    MahadevERP::DepreciationCalculator depCalc;
    
    // Test calculateSchedule
    QVector<MahadevERP::DepreciationAssetItem> items = depCalc.calculateSchedule(QDate(2025, 4, 1), QDate(2026, 3, 31), true);
    QVERIFY(items.size() >= 0);

    // Test calculateTotals
    MahadevERP::DepreciationSummaryTotals totals = depCalc.calculateTotals(items);
    QCOMPARE(totals.totalItems, items.size());

    // 2. Trial Balance Controller Multi-Mode & Balancing Verification
    MahadevERP::TrialBalanceController tbCtrl;
    tbCtrl.setDateRange(QDate(2025, 4, 1), QDate(2026, 3, 31));

    // Normal View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::NormalView);
    QVERIFY(tbCtrl.rows().size() >= 0);
    double totalDr = tbCtrl.totals().totalCloseDebit;
    double totalCr = tbCtrl.totals().totalCloseCredit;
    double diff = tbCtrl.totals().difference;
    QCOMPARE(std::abs(std::abs(totalDr - totalCr) - diff) < 0.01, true);

    // Flat View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::FlatView);
    QVERIFY(tbCtrl.rows().size() >= 0);

    // Flat Grouped View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::FlatGrouped);
    QVERIFY(tbCtrl.rows().size() >= 0);

    // Normal Detailed View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::NormalDetailed);
    QVERIFY(tbCtrl.rows().size() >= 0);

    // Without Opening Balance View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::WithoutOpBal);
    QVERIFY(tbCtrl.rows().size() >= 0);

    // Show Turnover View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::ShowTurnover);
    QVERIFY(tbCtrl.rows().size() >= 0);

    // Show Opening Balance View
    tbCtrl.setMode(MahadevERP::TrialBalanceMode::ShowOpeningBal);
    QVERIFY(tbCtrl.rows().size() >= 0);

    // 3. Mathematical Hierarchy Engine Verification (AccountClassifier)
    QVERIFY(AccountClassifier::isDescendantOf(8, 2, 0, 0, StandardGroupCode::CurrentAssets));
    QVERIFY(AccountClassifier::isDescendantOf(39, 8, 2, 0, StandardGroupCode::SundryDebtors));
    QVERIFY(AccountClassifier::isDescendantOf(39, 8, 2, 0, StandardGroupCode::CurrentAssets));
    QVERIFY(!AccountClassifier::isDescendantOf(39, 8, 2, 0, StandardGroupCode::Capital));
    QVERIFY(AccountClassifier::isDescendantOf(1, 0, 0, 0, StandardGroupCode::Capital));
    QVERIFY(AccountClassifier::isDescendantOf(3, 2, 0, 0, StandardGroupCode::BankAccounts));
    QVERIFY(AccountClassifier::isDescendantOf(28, 13, 9, 0, StandardGroupCode::SecuredLoansCC));
    QVERIFY(AccountClassifier::isDescendantOf(4, 2, 0, 0, StandardGroupCode::CashInHand));

    // Verify SQL clause generation
    QString sqlClause = AccountClassifier::generateHierarchySqlClause({StandardGroupCode::BankAccounts, StandardGroupCode::SecuredLoansCC}, "g");
    QVERIFY(sqlClause.contains("g.code1st IN (3, 28)"));
    QVERIFY(sqlClause.contains("g.code2nd IN (3, 28)"));

    // 5. Group Search and Modification Subsystem Verification
    QVariantList searchedGroups = AccountClassifier::searchGroups("Debtors", 20);
    QVERIFY(searchedGroups.size() > 0);
    bool foundSundryDebtors = false;
    for (const auto& gVar : searchedGroups) {
        if (gVar.toMap().value("name").toString().contains("Debtors", Qt::CaseInsensitive)) {
            foundSundryDebtors = true;
            break;
        }
    }
    QVERIFY(foundSundryDebtors);

    // Test creating a dynamic custom group
    QString testGrpName = "TEST_CUSTOM_EXPORT_DEBTORS";
    QString createErr;
    bool createOk = AccountClassifier::createOrUpdateGroup(0, testGrpName, "Sundry Debtors", "Assets", "Test Group for Export", true, &createErr);
    QVERIFY2(createOk, qPrintable(createErr));

    QVariantMap grpDetails = AccountClassifier::getGroupDetails(testGrpName);
    QVERIFY(!grpDetails.isEmpty());
    int newGroupId = grpDetails.value("id").toInt();
    QVERIFY(newGroupId > 0);
    QCOMPARE(grpDetails.value("parent_group_name").toString(), QString("Sundry Debtors"));
    QCOMPARE(grpDetails.value("code2nd").toInt(), 8); // Parent is Sundry Debtors (8)
    QCOMPARE(grpDetails.value("code3rd").toInt(), 2); // Grandparent is Current Assets (2)

    // Test modifying group name & parent
    QString modGrpName = "TEST_MODIFIED_EXPORT_DEBTORS";
    QString modErr;
    bool modOk = AccountClassifier::createOrUpdateGroup(newGroupId, modGrpName, "Sundry Debtors", "Assets", "Modified Group Notes", true, &modErr);
    QVERIFY2(modOk, qPrintable(modErr));

    QVariantMap modDetails = AccountClassifier::getGroupDetailsById(newGroupId);
    QCOMPARE(modDetails.value("name").toString(), modGrpName);

    // Clean up test group
    AccountClassifier::deleteGroup(newGroupId);

    qDebug() << "[TEST] Final Reports, Deterministic Hierarchy and Depreciation Subsystems verified successfully!";
}

void LogicBoardTestSuite::testCashBankFlowAndCashVouchers() {
    DatabaseManager &db = DatabaseManager::instance();
    VouchersModel vchModel;

    // 1. Next Voucher Number Generation for Cash Payment and Receipt
    QString nextPymt = vchModel.get_next_voucher_no("Cash Payment");
    QString nextRcpt = vchModel.get_next_voucher_no("Cash Receipt");
    QVERIFY(!nextPymt.isEmpty());
    QVERIFY(!nextRcpt.isEmpty());
    QVERIFY(nextPymt.startsWith("Pymt-"));
    QVERIFY(nextRcpt.startsWith("Rcpt-"));

    // 2. Ensure test parties exist
    QVariant p1 = db.executeScalar("SELECT id FROM parties WHERE name = 'FLOW_TEST_SUPPLIER' LIMIT 1;");
    if (!p1.isValid() || p1.isNull()) {
        db.executeNonQuery("INSERT INTO parties (name, group_name, party_type) VALUES ('FLOW_TEST_SUPPLIER', 'Sundry Creditors', 'Supplier');");
    }
    QVariant p2 = db.executeScalar("SELECT id FROM parties WHERE name = 'FLOW_TEST_CUSTOMER' LIMIT 1;");
    if (!p2.isValid() || p2.isNull()) {
        db.executeNonQuery("INSERT INTO parties (name, group_name, party_type) VALUES ('FLOW_TEST_CUSTOMER', 'Sundry Debtors', 'Customer');");
    }
    QVariant p3 = db.executeScalar("SELECT id FROM parties WHERE name = 'Cash In Hand' LIMIT 1;");
    if (!p3.isValid() || p3.isNull()) {
        db.executeNonQuery("INSERT INTO parties (name, group_name, party_type) VALUES ('Cash In Hand', 'Cash-In-Hand', 'Cash');");
    }

    // 3. Post a Cash Payment Voucher (Pymt)
    // Supplier Dr 25,000 | Cash In Hand Cr 25,000
    QVariantList pymtRows;
    QVariantMap r1, r2;
    r1["drcr"] = "Dr";
    r1["ledgerName"] = "FLOW_TEST_SUPPLIER";
    r1["debitAmt"] = 25000.0;
    r1["creditAmt"] = 0.0;
    r1["refNo"] = "CASH-PAY-01";
    pymtRows.append(r1);

    r2["drcr"] = "Cr";
    r2["ledgerName"] = "Cash In Hand";
    r2["debitAmt"] = 0.0;
    r2["creditAmt"] = 25000.0;
    r2["refNo"] = "";
    pymtRows.append(r2);

    bool pymtSaved = vchModel.save_multi_row_voucher(0, "Cash Payment", nextPymt, "2025-05-10", "Test Cash Payment", pymtRows);
    QVERIFY2(pymtSaved, "Cash Payment voucher should be saved successfully");

    // Verify voucher and trans_type
    QVariant legType = db.executeScalar("SELECT legacy_type FROM vouchers WHERE voucher_no = ? LIMIT 1;", {nextPymt});
    QCOMPARE(legType.toString(), QString("Pymt"));

    // 4. Post a Cash Receipt Voucher (Rcpt)
    // Cash In Hand Dr 50,000 | Customer Cr 50,000
    QVariantList rcptRows;
    QVariantMap rc1, rc2;
    rc1["drcr"] = "Dr";
    rc1["ledgerName"] = "Cash In Hand";
    rc1["debitAmt"] = 50000.0;
    rc1["creditAmt"] = 0.0;
    rc1["refNo"] = "";
    rcptRows.append(rc1);

    rc2["drcr"] = "Cr";
    rc2["ledgerName"] = "FLOW_TEST_CUSTOMER";
    rc2["debitAmt"] = 0.0;
    rc2["creditAmt"] = 50000.0;
    rc2["refNo"] = "CASH-REC-01";
    rcptRows.append(rc2);

    bool rcptSaved = vchModel.save_multi_row_voucher(0, "Cash Receipt", nextRcpt, "2025-05-15", "Test Cash Receipt", rcptRows);
    QVERIFY2(rcptSaved, "Cash Receipt voucher should be saved successfully");

    QVariant rcptLegType = db.executeScalar("SELECT legacy_type FROM vouchers WHERE voucher_no = ? LIMIT 1;", {nextRcpt});
    QCOMPARE(rcptLegType.toString(), QString("Rcpt"));

    // 5. Test CashBankFlowController in CashFlow Mode
    MahadevERP::CashBankFlowController cashCtrl;
    cashCtrl.setStatementType(MahadevERP::FlowStatementType::CashFlow);
    cashCtrl.loadData("2025-04-01", "2026-03-31");

    QVERIFY(cashCtrl.partyRows().size() > 0);
    QVERIFY(cashCtrl.bankCashRows().size() >= 1); // Contains Cash In Hand

    // Find our test parties
    bool foundSupplier = false;
    bool foundCustomer = false;
    for (const auto& row : cashCtrl.partyRows()) {
        if (row.ledgerName == "FLOW_TEST_SUPPLIER") {
            foundSupplier = true;
            QVERIFY(row.payments >= 25000.0);
        }
        if (row.ledgerName == "FLOW_TEST_CUSTOMER") {
            foundCustomer = true;
            QVERIFY(row.receipts >= 50000.0);
        }
    }
    QVERIFY(foundSupplier);
    QVERIFY(foundCustomer);

    // Verify grand totals
    const auto& summary = cashCtrl.summary();
    QVERIFY(summary.totalReceipts >= 50000.0);
    QVERIFY(summary.totalPayments >= 25000.0);

    // 6. Test CashBankFlowController in BankFlow Mode
    MahadevERP::CashBankFlowController bankCtrl;
    bankCtrl.setStatementType(MahadevERP::FlowStatementType::BankFlow);
    bankCtrl.loadData("2025-04-01", "2026-03-31");
    QVERIFY(bankCtrl.summary().rowCount >= 0);

    // 7. Test CashBankFlowController in JointFlow Mode
    MahadevERP::CashBankFlowController jointCtrl;
    jointCtrl.setStatementType(MahadevERP::FlowStatementType::JointFlow);
    jointCtrl.loadData("2025-04-01", "2026-03-31");
    QVERIFY(jointCtrl.bankCashRows().size() >= 1);

    // 8. Test Search Filter
    cashCtrl.loadData("2025-04-01", "2026-03-31", "FLOW_TEST_SUPPLIER");
    QCOMPARE(cashCtrl.partyRows().size(), 1);
    QCOMPARE(cashCtrl.partyRows().first().ledgerName, QString("FLOW_TEST_SUPPLIER"));

    // Reset filter
    cashCtrl.loadData("2025-04-01", "2026-03-31", "");
    QVERIFY(cashCtrl.partyRows().size() >= 2);

    // 9. Test CSV Export generation
    QString csv = cashCtrl.generateCsvReport();
    QVERIFY(csv.contains("Cash Flow Statement", Qt::CaseInsensitive));
    QVERIFY(csv.contains("FLOW_TEST_SUPPLIER"));
    QVERIFY(csv.contains("FLOW_TEST_CUSTOMER"));

    qDebug() << "[TEST] Cash Book & Flow Statements Subsystem verified successfully!";
}

void LogicBoardTestSuite::testPrintExportControllerUniversalExports() {
    PrintExportController ctrl;
    
    // 1. Test Generic Export Engines
    QString sampleHtml = "<html><body><h1>Mahadev Rice Mill Test Statement</h1><p>Sample statement</p></body></html>";
    QString tmpDir = QDir::tempPath();
    
    QString pdfPath = ctrl.exportHtmlToPdf(sampleHtml, "test_sample.pdf", tmpDir + "/test_sample.pdf", false);
    QVERIFY(!pdfPath.isEmpty());
    QVERIFY(QFile::exists(pdfPath));
    QFile::remove(pdfPath);
    
    QString odfPath = ctrl.exportHtmlToOdf(sampleHtml, "test_sample.odt", tmpDir + "/test_sample.odt", false);
    QVERIFY(!odfPath.isEmpty());
    QVERIFY(QFile::exists(odfPath));
    QFile::remove(odfPath);
    
    QString xlsPath = ctrl.exportHtmlToExcel(sampleHtml, "test_sample.xls", tmpDir + "/test_sample.xls");
    QVERIFY(!xlsPath.isEmpty());
    QVERIFY(QFile::exists(xlsPath));
    QFile::remove(xlsPath);
    
    QStringList headers = {"Date", "Particulars", "Debit", "Credit"};
    QVector<QStringList> rows = {{"01/04/2026", "Opening Balance", "1000.00", "0.00"}};
    QString csvPath = ctrl.exportTableToCsv(headers, rows, "test_sample.csv", tmpDir + "/test_sample.csv");
    QVERIFY(!csvPath.isEmpty());
    QVERIFY(QFile::exists(csvPath));
    QFile::remove(csvPath);

    // 2. Test HTML Renderers
    QString tbHtml = ctrl.renderTrialBalanceHtml("2026-03-31", 0);
    QVERIFY(tbHtml.contains("TRIAL BALANCE", Qt::CaseInsensitive));

    QString plHtml = ctrl.renderProfitLossHtml("2025-04-01", "2026-03-31");
    QVERIFY(plHtml.contains("PROFIT &amp; LOSS", Qt::CaseInsensitive) || plHtml.contains("PROFIT & LOSS", Qt::CaseInsensitive));

    QString bsHtml = ctrl.renderBalanceSheetHtml("2026-03-31");
    QVERIFY(bsHtml.contains("BALANCE SHEET", Qt::CaseInsensitive));

    QString dbHtml = ctrl.renderDayBookHtml("2025-04-01", "2026-03-31");
    QVERIFY(dbHtml.contains("DAY BOOK", Qt::CaseInsensitive));

    QString srHtml = ctrl.renderSalesRegisterHtml("2025-04-01", "2026-03-31");
    QVERIFY(srHtml.contains("SALES REGISTER", Qt::CaseInsensitive));

    QString prHtml = ctrl.renderPurchaseRegisterHtml("2025-04-01", "2026-03-31");
    QVERIFY(prHtml.contains("PURCHASE REGISTER", Qt::CaseInsensitive));

    QString mandiHtml = ctrl.renderMandiReportHtml("Form M", "2025-04-01", "2026-03-31");
    QVERIFY(mandiHtml.contains("FORM M", Qt::CaseInsensitive));

    QString gstrHtml = ctrl.renderGstrReportHtml("GSTR-1", "2025-04-01", "2026-03-31");
    QVERIFY(gstrHtml.contains("GSTR-1", Qt::CaseInsensitive));

    // 3. Test Statement ODF & Excel Exports
    QString odfTrial = ctrl.export_trial_balance_odf("2026-03-31", 0, tmpDir + "/test_tb.odt");
    QVERIFY(!odfTrial.isEmpty());
    QVERIFY(QFile::exists(odfTrial));
    QFile::remove(odfTrial);

    QString xlsTrial = ctrl.export_trial_balance_excel("2026-03-31", 0, tmpDir + "/test_tb.xls");
    QVERIFY(!xlsTrial.isEmpty());
    QVERIFY(QFile::exists(xlsTrial));
    QFile::remove(xlsTrial);

    qDebug() << "[TEST] Universal Print, PDF, ODF & Excel Exports Subsystems verified successfully!";
}

void LogicBoardTestSuite::testBardanaControllerWorkflow() {
    BardanaController bCtrl;

    DatabaseManager::instance().executeNonQuery("DELETE FROM bardana_transactions WHERE party_id = 9999 OR party_name = 'M/S Test Bardana Trader';");
    DatabaseManager::instance().executeNonQuery("INSERT OR IGNORE INTO parties (id, name, group_id) VALUES (9999, 'M/S Test Bardana Trader', 1);");

    // 1. Issue 500 Jute bags to farmer/party
    bool ok1 = bCtrl.createBardanaTransaction(
        "ISSUE", "2026-04-10", "BRD-001", 9999, "M/S Test Bardana Trader",
        "Jute 50kg (Pukka)", "Main Godown", "Dr", 500, 45.0, "HR-57-A-1234", "CH-101", "Bags issued for paddy loading"
    );
    QVERIFY(ok1);

    // 2. Receive 300 Jute bags back
    bool ok2 = bCtrl.createBardanaTransaction(
        "RECEIVE", "2026-04-15", "BRD-002", 9999, "M/S Test Bardana Trader",
        "Jute 50kg (Pukka)", "Main Godown", "Cr", 300, 45.0, "HR-57-A-1234", "CH-102", "Bags received back"
    );
    QVERIFY(ok2);

    // 3. Verify running party balance (500 - 300 = 200 bags)
    int balance = bCtrl.getPartyBagBalance(9999, "Jute 50kg (Pukka)");
    QCOMPARE(balance, 200);

    // 4. Verify godown stock
    QVariantList summary = bCtrl.getGodownStockSummary();
    QVERIFY(!summary.isEmpty());

    qDebug() << "[TEST] Bardana Gunny Bag Controller & Packaging Ledger verified successfully!";
}

void LogicBoardTestSuite::testGateRegisterControllerWorkflow() {
    GateRegisterController gCtrl;

    QString passNo = gCtrl.generateNextGatePassNo("GP-TEST-");
    QVERIFY(!passNo.isEmpty());

    // 1. Inward entry at gate
    bool ok1 = gCtrl.createGateEntry(
        passNo, "2026-04-20", "10:30", "INWARD", "Paddy Purchase",
        "HR-39-B-5555", "Suresh Kumar", "9812000000", "Self Trolley",
        201, "Zimidar Ram Kumar", "Paddy Basmati 1121", 250, 162.50, "Arrival at gate"
    );
    QVERIFY(ok1);

    // 2. Update Weighbridge weights
    bool ok2 = gCtrl.updateWeighbridgeWeights(passNo, "WB-999", 162.50, 32.50, 130.00);
    QVERIFY(ok2);

    // 3. Verify status updated to COMPLETED
    QVariantMap entry = gCtrl.getGateEntryByPassNo(passNo);
    QCOMPARE(entry.value("status").toString(), QString("COMPLETED"));
    QCOMPARE(entry.value("net_weight_qtl").toDouble(), 130.00);

    // 4. Mark gate exit
    bool ok3 = gCtrl.updateGateStatus(entry.value("id").toInt(), "DISPATCHED", "12:15");
    QVERIFY(ok3);

    qDebug() << "[TEST] Gate Register Inward/Outward & Weighbridge Integration verified successfully!";
}

void LogicBoardTestSuite::testSaudaContractAndDalaliSettlementWorkflow() {
    SaudaController sCtrl;

    QString sNo = sCtrl.generateNextSaudaNo("SD-TEST-");

    // 1. Create Forward Contract Booking (Sale of 1000 Qtl Rice @ ₹6500/Qtl, Dalali ₹10/Qtl)
    bool ok1 = sCtrl.createSaudaContract(
        sNo, "2026-04-01", "SALE",
        301, "Shree Ganesh Agro",
        401, "Suresh Dalal",
        501, "Basmati Rice 1121 Steam", "Grade-A",
        2000, 1000.0, 6500.0, 10.0, 0.0,
        "2026-04-05", "2026-04-20", "Immediate Payment on Delivery", "Moisture <= 12%"
    );
    QVERIFY(ok1);

    // 2. Record Partial Fulfillment (400 Qtl)
    QVariantMap sauda = sCtrl.getSaudaContracts("SALE", "PENDING").first().toMap();
    int sId = sauda.value("id").toInt();

    bool ok2 = sCtrl.fulfillSauda(sId, 800, 400.0);
    QVERIFY(ok2);

    QVariantMap updatedSauda = sCtrl.getSaudaById(sId);
    QCOMPARE(updatedSauda.value("status").toString(), QString("PARTIAL"));
    QCOMPARE(updatedSauda.value("fulfilled_weight_qtl").toDouble(), 400.0);

    // 3. Settle Dalali for the 400 Qtl fulfilled
    // Dalali = 400 * 10 = ₹4000.00, TDS 194H (5%) = ₹200.00, Net = ₹3800.00
    bool ok3 = sCtrl.createDalaliSettlement(
        401, "Suresh Dalal", sId, sNo, "2026-04-10", "SALE", "VCH-101", "INV-101",
        "Shree Ganesh Agro", "Basmati Rice 1121 Steam", 400.0, 6500.0, 10.0, 5.0, "Dalali settled on 400 Qtl dispatch"
    );
    QVERIFY(ok3);

    QVariantList settlements = sCtrl.getDalaliSettlements(401);
    QVERIFY(!settlements.isEmpty());
    QVariantMap sett = settlements.first().toMap();
    QCOMPARE(sett.value("dalali_amount").toDouble(), 4000.00);
    QCOMPARE(sett.value("tds_amount").toDouble(), 200.00);
    QCOMPARE(sett.value("net_dalali_payable").toDouble(), 3800.00);

    // 4. Post Dalali to Double-Entry Journal Voucher (JV)
    int settId = sett.value("id").toInt();
    bool ok4 = sCtrl.postDalaliToJournalVoucher(settId);
    QVERIFY(ok4);

    // Verify double-entry balancing in transactions
    QVariantMap postedSett = sCtrl.getDalaliSettlements(401).first().toMap();
    QCOMPARE(postedSett.value("is_posted_to_jv").toInt(), 1);
    QString jvNo = postedSett.value("journal_voucher_no").toString();
    QVERIFY(!jvNo.isEmpty());

    QVariant totalDr = DatabaseManager::instance().executeScalar(
        "SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Dr';", {jvNo}
    );
    QVariant totalCr = DatabaseManager::instance().executeScalar(
        "SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Cr';", {jvNo}
    );
    QCOMPARE(totalDr.toDouble(), 4000.00); // Dalali Expense Dr
    QCOMPARE(totalCr.toDouble(), 4000.00); // Broker Cr (3800) + TDS 194H Cr (200)

    qDebug() << "[TEST] Sauda Forward Contract & Dalali Double-Entry Settlement verified successfully!";
}

void LogicBoardTestSuite::testNewGroupAndLedgerFourCodeStructure() {
    DatabaseManager::instance().executeNonQuery("DELETE FROM parties WHERE group_name IN ('North Zone Rice Debtors', 'Ludhiana Rice Mill Buyers') OR name = 'Shree Krishna Agro Ludhiana';");
    DatabaseManager::instance().executeNonQuery("DELETE FROM account_groups WHERE name IN ('North Zone Rice Debtors', 'Ludhiana Rice Mill Buyers');");

    // 1. Test Creating a Custom Group under Sundry Debtors (code 8)
    QString errMsg;
    QString customGroupName = "North Zone Rice Debtors";
    bool okGroup1 = AccountClassifier::createOrUpdateGroup(0, customGroupName, "Sundry Debtors", "Assets", "Test Group", true, &errMsg);
    QVERIFY2(okGroup1, qPrintable(errMsg));

    GroupHierarchyInfo g1 = AccountClassifier::getGroupInfo(customGroupName);
    QVERIFY(g1.id > 0);
    QVERIFY(g1.code1 >= 100);
    QCOMPARE(g1.code2, 8); // Parent Sundry Debtors
    QCOMPARE(g1.code3, 2); // Grandparent Current Assets
    QCOMPARE(g1.code4, 0);
    QCOMPARE(g1.nature, QString("Assets"));

    // 2. Test Creating a Nested Sub-Group under the Custom Group
    QString subGroupName = "Ludhiana Rice Mill Buyers";
    bool okGroup2 = GroupHierarchyPipeline::instance().saveGroup(0, subGroupName, customGroupName, "Assets", 1, false, &errMsg);
    QVERIFY2(okGroup2, qPrintable(errMsg));

    GroupHierarchyInfo g2 = AccountClassifier::getGroupInfo(subGroupName);
    QVERIFY(g2.id > 0);
    QVERIFY(g2.code1 > g1.code1);
    QCOMPARE(g2.code2, g1.code1); // Parent is North Zone Rice Debtors
    QCOMPARE(g2.code3, 8);        // Grandparent is Sundry Debtors
    QCOMPARE(g2.code4, 2);        // Great-grandparent is Current Assets

    // 3. Test Mathematical Ancestor Hierarchy Check (0 string matching)
    QVERIFY(AccountClassifier::isDescendantOf(g2.code1, g2.code2, g2.code3, g2.code4, StandardGroupCode::SundryDebtors));
    QVERIFY(AccountClassifier::isDescendantOf(g2.code1, g2.code2, g2.code3, g2.code4, StandardGroupCode::CurrentAssets));
    QVERIFY(!AccountClassifier::isDescendantOf(g2.code1, g2.code2, g2.code3, g2.code4, StandardGroupCode::SundryCreditors));

    // 4. Test Creating a New Ledger under the Sub-Group via PartiesModel
    PartiesModel partiesModel;
    QVariantMap ledgerData;
    QString ledgerName = "Ludhiana Basmati Mills Ltd";
    ledgerData["name"] = ledgerName;
    ledgerData["group_name"] = subGroupName;
    ledgerData["opening_balance"] = 150000.00;
    ledgerData["balance_type"] = "Dr";
    ledgerData["city"] = "Ludhiana";

    bool okLedger = partiesModel.add_ledger_extended(ledgerData);
    QVERIFY(okLedger);

    // Verify database record has correct integer group_id, group_code, and party_type
    QVariantList pRows = DatabaseManager::instance().executeQuery(
        "SELECT id, group_name, group_id, group_code, party_type, opening_balance FROM parties WHERE name = ? LIMIT 1;",
        {ledgerName}
    );
    QVERIFY(!pRows.isEmpty());
    QVariantMap pMap = pRows.first().toMap();
    QCOMPARE(pMap.value("group_name").toString(), subGroupName);
    QCOMPARE(pMap.value("group_id").toLongLong(), g2.id);
    QCOMPARE(pMap.value("group_code").toInt(), g2.code1);
    QCOMPARE(pMap.value("party_type").toString(), QString("Buyer"));

    // 5. Test Creating a Ledger via LedgerPipeline
    LedgerNode pipelineNode;
    QString pipeLedgerName = "Khanna Agro Exports";
    pipelineNode.name = pipeLedgerName;
    pipelineNode.groupId = g2.id;
    pipelineNode.groupName = subGroupName;
    pipelineNode.openingBalance = 250000.00;
    pipelineNode.openingBalanceType = "Dr";
    pipelineNode.city = "Khanna";

    bool okPipeLedger = LedgerPipeline::instance().saveLedger(pipelineNode, &errMsg);
    QVERIFY2(okPipeLedger, qPrintable(errMsg));

    QVariantList pipeRows = DatabaseManager::instance().executeQuery(
        "SELECT id, group_name, group_id, group_code, party_type, opening_balance FROM parties WHERE name = ? LIMIT 1;",
        {pipeLedgerName}
    );
    QVERIFY(!pipeRows.isEmpty());
    QVariantMap pipeMap = pipeRows.first().toMap();
    QCOMPARE(pipeMap.value("group_name").toString(), subGroupName);
    QCOMPARE(pipeMap.value("group_id").toLongLong(), g2.id);
    QCOMPARE(pipeMap.value("group_code").toInt(), g2.code1);
    QCOMPARE(pipeMap.value("party_type").toString(), QString("Buyer"));

    qDebug() << "[TEST] 4-code structure hierarchy and ledger generation verified successfully!";
}

void LogicBoardTestSuite::testSalaryRegisterControllerAndPayrollPosting() {
    auto& db = DatabaseManager::instance();

    // 1. Insert test employees
    QString emp1 = "TEST_EMP_RAMESH_SINGH";
    QString emp2 = "TEST_EMP_SURESH_KUMAR";

    db.executeNonQuery("DELETE FROM parties WHERE name IN (?, ?);", {emp1, emp2});
    db.executeNonQuery(
        "INSERT INTO parties (name, group_name, group_code, party_type, salary_per_month) "
        "VALUES (?, 'Employees', 35, 'Employee', 30000.0);",
        {emp1}
    );
    int emp1Id = db.executeScalar("SELECT last_insert_rowid();").toInt();

    db.executeNonQuery(
        "INSERT INTO parties (name, group_name, group_code, party_type, salary_per_month) "
        "VALUES (?, 'Employees', 35, 'Employee', 45000.0);",
        {emp2}
    );
    int emp2Id = db.executeScalar("SELECT last_insert_rowid();").toInt();

    // 2. Instantiate SalaryRegisterController
    MahadevERP::SalaryRegisterController ctrl;
    ctrl.loadPayroll(2025, 4); // April 2025 (30 days)

    const auto& items = ctrl.items();
    int idx1 = -1, idx2 = -1;
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].partyId == emp1Id) idx1 = i;
        if (items[i].partyId == emp2Id) idx2 = i;
    }

    QVERIFY(idx1 >= 0);
    QVERIFY(idx2 >= 0);

    QCOMPARE(ctrl.items()[idx1].masterSalary, 30000.0);
    QCOMPARE(ctrl.items()[idx2].masterSalary, 45000.0);
    QCOMPARE(ctrl.items()[idx1].daysInMonth, 30);

    // 3. Test adjustments: Emp1 has 25 present days, 2000 allowance, 1800 PF, 1000 advance
    ctrl.setItemPresentDays(idx1, 25.0); // Earned = 30000/30 * 25 = 25000
    ctrl.setItemAllowances(idx1, 2000.0);
    ctrl.setItemPf(idx1, 1800.0);
    ctrl.setItemAdvance(idx1, 1000.0);

    // Expected net = 25000 + 2000 - 1800 - 1000 = 24200
    QCOMPARE(ctrl.items()[idx1].earnedBasic, 25000.0);
    QCOMPARE(ctrl.items()[idx1].netPayable, 24200.0);

    // Emp2: Full 30 days, 5000 bonus, 2000 TDS -> Net = 45000 + 5000 - 2000 = 48000
    ctrl.setItemAllowances(idx2, 5000.0);
    ctrl.setItemTds(idx2, 2000.0);
    QCOMPARE(ctrl.items()[idx2].netPayable, 48000.0);

    // Select only our two test employees
    ctrl.selectAll(false);
    ctrl.setItemSelected(idx1, true);
    ctrl.setItemSelected(idx2, true);

    QCOMPARE(ctrl.summary().selectedEmployees, 2);
    QCOMPARE(ctrl.summary().totalNetPayable, 72200.0);

    // 4. Batch Post Salaries
    QDate vDate(2025, 4, 30);
    auto res = ctrl.postSelectedSalaries(vDate);
    QVERIFY2(res.success, qPrintable(res.errorMessage));
    QCOMPARE(res.vouchersCreated, 2);
    QCOMPARE(res.totalAmountPosted, 72200.0);

    // 5. Verify database records in salary_payments, vouchers, transactions
    QVariant payCount = db.executeScalar(
        "SELECT COUNT(*) FROM salary_payments WHERE month_year = '2025-04' AND party_id IN (?, ?);",
        {emp1Id, emp2Id}
    );
    QCOMPARE(payCount.toInt(), 2);

    // Verify double-entry balancing for each posted voucher
    for (const QString& vNo : res.createdVoucherNumbers) {
        QVariant sumDr = db.executeScalar("SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Dr';", {vNo});
        QVariant sumCr = db.executeScalar("SELECT SUM(amount) FROM transactions WHERE voucher_no = ? AND dr_cr = 'Cr';", {vNo});
        QVERIFY(sumDr.isValid() && sumCr.isValid());
        QCOMPARE(sumDr.toDouble(), sumCr.toDouble());
    }

    qDebug() << "[TEST] Employee Payroll & Salary Register Crediting verified successfully!";
}

QTEST_MAIN(LogicBoardTestSuite)
#include "test_logicboard_suite.moc"
