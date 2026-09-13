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
#include "../src/models/tds_model.h"
#include "../src/models/stock_items_model.h"
#include "../src/models/account_groups_model.h"
#include "../src/models/generic_list_model.h"
#include "../src/services/canara_bank_statement_parser.h"
#include "../src/services/bank_statement_excel_parser.h"
#include "../src/models/bank_statement_controller.h"
#include "../src/models/transport_dispatch_controller.h"
#include "../src/models/debit_credit_note_controller.h"
#include "../src/models/firm_manager.h"
#include "../src/engine/bahi_khata_migrator.h"
#include "../src/database_manager.h"
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>

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

    // 3. Voucher Controllers & ViewModels Tests
    void testSalesVoucherControllerCalculations();
    void testPurchaseVoucherControllerCalculations();
    void testJournalVoucherDebitCreditBalanceCheck();
    void testChequeVoucherModes();
    void testTdsCalculationsWithSurchargeAndCess();
    void testJFormYieldAndDeductionCalculations();
    void testMillingBatchYieldDistribution();
    void testPaddyProcurementMoistureDeductions();
    void testPerFinancialYearVoucherNumbering();

    // 4. Masters & Regulatory Validations
    void testGstinValidationAndPanExtraction();
    void testStockItemOpeningValuation();

    // 5. Weighbridge & Transport Logistics
    void testTransportDispatchController();

    // 6. GST Debit & Credit Notes
    void testDebitCreditNoteController();

    // 7. Firm & Fiscal Years Engine
    void testFirmTypeResolutionAndDynamicFiscalYears();

    // 8. View Compilation & Statement Parsers
    void testCanaraBankStatementPdfParser();
    void testNativeXlsBankStatementParser();
    void testBankStatementControllerPosting();
    void testAllQmlViewsInstantiable();
};

void LogicBoardTestSuite::initTestCase() {
    qDebug() << "[TEST INIT] Initializing LogicBoard Test Suite...";
    QString dbPath = "data/mahadev_rice_industry_data_004.db";
    if (QFile::exists("../data/mahadev_rice_industry_data_004.db")) {
        dbPath = "../data/mahadev_rice_industry_data_004.db";
    }
    DatabaseManager::instance().initDatabase(dbPath);
}

void LogicBoardTestSuite::cleanupTestCase() {
    qDebug() << "[TEST CLEANUP] LogicBoard Test Suite Completed.";
}

// -------------------------------------------------------------
// 1. Date Engine Tests
// -------------------------------------------------------------
void LogicBoardTestSuite::testDateParsingAndFormatting() {
    auto &dateSvc = AccountingDateService::instance();

    // Dot separated short year
    QCOMPARE(dateSvc.resolveDate("1.3.26"), QString("01/03/2026"));
    QCOMPARE(dateSvc.resolveDate("15.08.25"), QString("15/08/2025"));

    // Dash separated
    QCOMPARE(dateSvc.resolveDate("1-3-2026"), QString("01/03/2026"));
    QCOMPARE(dateSvc.resolveDate("01-03-2026"), QString("01/03/2026"));

    // Slash separated
    QCOMPARE(dateSvc.resolveDate("1/3/26"), QString("01/03/2026"));
    QCOMPARE(dateSvc.resolveDate("31/12/2025"), QString("31/12/2025"));

    // ISO conversions
    QCOMPARE(dateSvc.toIso("01/03/2026"), QString("2026-03-01"));
    QCOMPARE(dateSvc.toIso("15-08-2025"), QString("2025-08-15"));
    QCOMPARE(dateSvc.fromIso("2026-03-01"), QString("01/03/2026"));
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
// 3. Voucher Controllers & ViewModels Tests
// -------------------------------------------------------------
void LogicBoardTestSuite::testSalesVoucherControllerCalculations() {
    SalesVoucherController ctrl;
    ctrl.resetForm("01/04/2026");

    // Add 2 line items:
    // 1) 100 bags, 50.0 Qtl @ 4000.00 => 2,00,000.00
    // 2) 50 bags, 25.0 Qtl @ 3800.00 => 95,000.00
    ctrl.lineItemsModel()->appendRow("Rice Basmati 1121", "1006", "QTL", 100, 50.0, 50.0, 4000.0, 200000.0);
    ctrl.lineItemsModel()->appendRow("Rice Sona Masoori", "1006", "QTL", 50, 50.0, 25.0, 3800.0, 95000.0);

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
    // For FY 2026-27 (244 existing invoices, max 245), next voucher must be Sale-246, next invoice MRI/2627-245
    QString vch2627 = salesModel.get_next_voucher_no("FY 2026-27");
    QCOMPARE(vch2627, QString("Sale-246"));
    QString inv2627 = salesModel.get_next_invoice_no("FY 2026-27");
    QCOMPARE(inv2627, QString("MRI/2627-245"));

    // For FY 2025-26 (1090 existing invoices, max 1093), next voucher must be Sale-1094
    QString vch2526 = salesModel.get_next_voucher_no("FY 2025-26");
    QCOMPARE(vch2526, QString("Sale-1094"));

    // Date string resolution: 01-05-2026 belongs to FY 2026-27
    QString vchByDate = salesModel.get_next_voucher_no("01-05-2026");
    QCOMPARE(vchByDate, QString("Sale-246"));
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

    // Switch back to original DB
    QString originalDb = "data/mahadev_rice_industry_data_004.db";
    if (QFile::exists("../data/mahadev_rice_industry_data_004.db")) originalDb = "../data/mahadev_rice_industry_data_004.db";
    DatabaseManager::instance().switchDatabase(originalDb);
}

extern int qInitResources_MahadevRiceMillERP_raw_qml_0();

void LogicBoardTestSuite::testAllQmlViewsInstantiable() {
    qInitResources_MahadevRiceMillERP_raw_qml_0();

    QQmlEngine engine;
    qmlRegisterType<GenericListModel>("MahadevERP", 1, 0, "GenericListModel");

    PartiesModel partiesModel;
    VouchersModel vouchersModel;
    SalesModel salesModel;
    PurchaseModel purchaseModel;
    FinancialYearsModel financialYearsModel;
    MillingModel millingModel;
    JFormModel jformModel;
    TdsModel tdsModel;
    StockItemsModel stockItemsModel;
    AccountGroupsModel accountGroupsModel;
    BankStatementController bankStatementCtrl;
    TransportDispatchController transportDispatchCtrl;
    DebitCreditNoteController debitCreditNoteCtrl;

    engine.rootContext()->setContextProperty("partiesModel", &partiesModel);
    engine.rootContext()->setContextProperty("vouchersModel", &vouchersModel);
    engine.rootContext()->setContextProperty("salesModel", &salesModel);
    engine.rootContext()->setContextProperty("purchaseModel", &purchaseModel);
    engine.rootContext()->setContextProperty("financialYearsModel", &financialYearsModel);
    engine.rootContext()->setContextProperty("millingModel", &millingModel);
    engine.rootContext()->setContextProperty("jformModel", &jformModel);
    engine.rootContext()->setContextProperty("tdsModel", &tdsModel);
    engine.rootContext()->setContextProperty("stockItemsModel", &stockItemsModel);
    engine.rootContext()->setContextProperty("accountGroupsModel", &accountGroupsModel);
    engine.rootContext()->setContextProperty("bankStatementCtrl", &bankStatementCtrl);
    engine.rootContext()->setContextProperty("transportDispatchCtrl", &transportDispatchCtrl);
    engine.rootContext()->setContextProperty("debitCreditNoteCtrl", &debitCreditNoteCtrl);

    QStringList viewsToTest = {
        "ChequeVoucherView.qml",
        "JournalVoucherView.qml",
        "SalesVoucherView.qml",
        "PurchaseVoucherView.qml",
        "JFormVoucherView.qml",
        "MillingVoucherView.qml",
        "TdsVoucherView.qml",
        "NewLedgerView.qml",
        "ModifyLedgerView.qml",
        "NewStockItemView.qml",
        "ModifyStockItemView.qml",
        "DashboardView.qml",
        "PaddyProcurementView.qml",
        "MillingView.qml",
        "SalesInvoicingView.qml",
        "VoucherLedgerView.qml",
        "ReportsView.qml",
        "SalesRegisterView.qml",
        "PurchaseRegisterView.qml",
        "MillingStatementView.qml",
        "StockDetailView.qml",
        "ViewLedgerStatementView.qml",
        "BankStatementImportView.qml",
        "TransportDispatchRegisterView.qml",
        "DebitCreditNoteView.qml"
    };

    for (const QString& vName : viewsToTest) {
        QUrl qrcUrl(QString("qrc:/MahadevERP/qml/views/%1").arg(vName));
        QQmlComponent comp(&engine, qrcUrl);
        if (!comp.isReady()) {
            qCritical() << "Failed to load view:" << vName << comp.errors();
        }
        QVERIFY2(comp.isReady(), qPrintable(QString("Failed to compile view: %1 (errors: %2)").arg(vName, comp.errorString())));
        QObject* obj = comp.create();
        QVERIFY2(obj != nullptr, qPrintable(QString("Failed to instantiate view: %1").arg(vName)));
        delete obj;
    }
}

QTEST_MAIN(LogicBoardTestSuite)
#include "test_logicboard_suite.moc"
