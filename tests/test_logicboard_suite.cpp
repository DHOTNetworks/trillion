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
#include "../src/database_manager.h"

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

    // 4. Masters & Regulatory Validations
    void testGstinValidationAndPanExtraction();
    void testStockItemOpeningValuation();
};

void LogicBoardTestSuite::initTestCase() {
    qDebug() << "[TEST INIT] Initializing LogicBoard Test Suite...";
    // Initialize temporary database if needed
    QDir().mkpath("data");
    DatabaseManager::instance().initDatabase("data/mahadev_rice_industry_data_004.db");
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

QTEST_MAIN(LogicBoardTestSuite)
#include "test_logicboard_suite.moc"
