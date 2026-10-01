#include <QTest>
#include <QObject>
#include "../src/engine/mandi_calculator.h"
#include "../src/engine/aank_interest_engine.h"
#include "../src/engine/gst_tax_engine.h"
#include "../src/engine/milling_yield_engine.h"
#include "../src/engine/gstr2_reconciler.h"
#include "../src/engine/gstr3b_engine.h"
#include "../src/engine/tds_fvu_exporter.h"
#include "../src/engine/gstr9_engine.h"
#include "../src/engine/busy_data_migrator.h"
#include "../src/database_manager.h"

using namespace MahadevERP;

class TestEnginesSuite : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testMandiCalculatorMath();
    void testAankInterestEngineMath();
    void testGstTaxEnginePosAndTcs();
    void testMillingYieldEngine();
    void testGstr2Reconciliation4WayMatch();
    void testGstr3BEngineCalculationAndExcelExport();
    void testTdsFvuExporter();
    void testGstr9AnnualReturnEngine();
    void testBusyDataMigratorInspectionAndMigration();
};

void TestEnginesSuite::initTestCase() {
    DatabaseManager::instance().initDatabase("data/mahadev_rice_industry_data_002.db");
}

void TestEnginesSuite::testMandiCalculatorMath() {
    MandiConfig cfg;
    cfg.damiRate = 2.0;
    cfg.marketFeeRate = 2.0;
    cfg.hrdfRate = 2.0;
    cfg.labourRatePerBag = 4.0;
    cfg.tareWeightPerBagKg = 0.50; // 500g bag tare
    cfg.firstTotalRoundOff = true;
    cfg.damiRoundOff = true;
    cfg.marketFeeRoundOff = true;
    cfg.hrdfRoundOff = true;
    cfg.labourRoundOff = true;
    cfg.finalInvoiceRoundOff = true;

    // 200 bags of Paddy @ 65kg packing = 130.00 Qtl gross
    // 200 * 0.50kg tare = 1.00 Qtl tare -> Net 129.00 Qtl
    // Rate = ₹3500/Qtl -> Basic = 129.00 * 3500 = ₹4,51,500.00
    MandiCalculationResult res = MandiCalculator::computeMandiTransaction(
        200.0,  // bags
        65.0,   // packing kg
        3500.0, // rate
        14.0,   // moisture pct (within 14% base limit)
        cfg
    );

    QCOMPARE(res.grossWeightQtl, 130.00);
    QCOMPARE(res.tareWeightQtl, 1.00);
    QCOMPARE(res.moistureDeductionQtl, 0.00);
    QCOMPARE(res.netWeightQtl, 129.00);
    QCOMPARE(res.basicAmount, 451500.00);

    // Dami (2%) = 451500 * 0.02 = 9030.00
    QCOMPARE(res.damiAmount, 9030.00);
    // Market Fee (2%) = 9030.00
    QCOMPARE(res.marketFeeAmount, 9030.00);
    // HRDF (2%) = 9030.00
    QCOMPARE(res.hrdfAmount, 9030.00);
    // Labour = 200 * 4 = 800.00
    QCOMPARE(res.labourAmount, 800.00);

    // Taxable Value = Basic (451500) + Dami (9030) + Labour (800) = 461330.00
    QCOMPARE(res.taxableAmount, 461330.00);

    // Total Net Payable invariant check
    QVERIFY(res.netPayableAmount > 0.0);
}

void TestEnginesSuite::testAankInterestEngineMath() {
    // 100 days interest on ₹1,00,000 balance at 12% per annum
    // Aank = 1,00,000 * 100 = 1,00,00,000 (1 Crore Product)
    // Interest = 1,00,00,000 * 12 / 36500 = ₹3287.67
    double product = 100000.0 * 100.0;
    double interest = AankInterestEngine::computeInterest(product, 12.0, false);
    QCOMPARE(interest, 3287.67);

    // Full statement test
    QList<QVariantMap> vchs;
    QVariantMap v1;
    v1["date"] = "2026-04-15";
    v1["voucher_no"] = "101";
    v1["voucher_type"] = "SL";
    v1["narration"] = "Sale of Rice";
    v1["debit"] = 50000.0;
    v1["credit"] = 0.0;
    vchs.append(v1);

    AankInterestStatement stmt = AankInterestEngine::calculateStatement(
        1, "Test Agro", 50000.0, "Dr",
        QDate(2026, 4, 1), QDate(2026, 4, 30),
        12.0, 12.0, vchs
    );

    QCOMPARE(stmt.entries.size(), 2); // Opening + 1 voucher
    QVERIFY(stmt.totalDrAank > 0.0);
    QVERIFY(stmt.totalDrInterest > 0.0);
}

void TestEnginesSuite::testGstTaxEnginePosAndTcs() {
    // Intra-State: Haryana (06) to Haryana (06)
    GstTaxBreakdown intra = GstTaxEngine::computeTax(
        100000.0, 5.0, "06", "06", "06"
    );
    QVERIFY(intra.isIntraState);
    QCOMPARE(intra.cgstAmount, 2500.00);
    QCOMPARE(intra.sgstAmount, 2500.00);
    QCOMPARE(intra.igstAmount, 0.00);
    QCOMPARE(intra.finalInvoiceValue, 105000.00);

    // Inter-State: Haryana (06) to Rajasthan (08)
    GstTaxBreakdown inter = GstTaxEngine::computeTax(
        100000.0, 5.0, "06", "08", "08"
    );
    QVERIFY(!inter.isIntraState);
    QCOMPARE(inter.cgstAmount, 0.00);
    QCOMPARE(inter.sgstAmount, 0.00);
    QCOMPARE(inter.igstAmount, 5000.00);
    QCOMPARE(inter.finalInvoiceValue, 105000.00);

    // TCS Sec 206C(1H) turnover > 50 Lakhs
    GstTaxBreakdown tcs = GstTaxEngine::computeTax(
        1000000.0, 5.0, "06", "08", "08", true, 6000000.0, 5000000.0
    );
    // 0.1% on ₹10,00,000 = ₹1,000.00
    QCOMPARE(tcs.tcsAmount, 1000.00);
}

void TestEnginesSuite::testMillingYieldEngine() {
    // 1000 Qtl Paddy input -> 670 Qtl Head Rice (67%), 70 Qtl Broken (7%), 80 Qtl Bran (8%), 170 Qtl Husk (17%)
    MillingBatchResult res = MillingYieldEngine::calculateBatchYield(
        "BATCH-01", "Basmati 1121",
        1000.0, 670.0, 70.0, 80.0, 170.0, 67.0
    );

    QCOMPARE(res.headRicePct, 67.00);
    QCOMPARE(res.brokenRicePct, 7.00);
    QCOMPARE(res.riceBranPct, 8.00);
    QCOMPARE(res.riceHuskPct, 17.00);
    QCOMPARE(res.totalRecoveryPct, 99.00);
    QCOMPARE(res.millingWastageQtl, 10.00); // 1% wastage = 10 Qtl
    QCOMPARE(res.yieldVariancePct, 0.00);
    QVERIFY(res.isWithinStandardTolerance);
}

void TestEnginesSuite::testGstr2Reconciliation4WayMatch() {
    QList<Gstr2BookRecord> books;
    Gstr2BookRecord b1;
    b1.supplierGstin = "06ABKFM5928Q1ZG";
    b1.invoiceNo = "INV/2026/00142";
    b1.taxableValue = 100000.00;
    b1.taxAmount = 5000.00;
    books.append(b1);

    QList<Gstr2PortalRecord> portal;
    Gstr2PortalRecord p1;
    p1.supplierGstin = "06ABKFM5928Q1ZG";
    p1.invoiceNo = "INV-2026-142"; // Different punctuation and leading zeros
    p1.taxableValue = 100000.00;
    p1.taxAmount = 5000.50; // within +/- 1.00 tolerance
    portal.append(p1);

    Gstr2ReconciliationSummary summary = Gstr2Reconciler::reconcile(books, portal, 1.0);
    QCOMPARE(summary.matchedCount, 1);
    QCOMPARE(summary.valueMismatchCount, 0);
    QCOMPARE(summary.notInPortalCount, 0);

    // Test with real Mahadev GSTR-2B JSON file
    QFile f("gst-data/returns_R2B_06ABKFM5928Q1ZG_042026.json");
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray jsonBytes = f.readAll();
        f.close();
        QList<Gstr2PortalRecord> portalRecords = Gstr2Reconciler::parseGstr2BJson(jsonBytes, "042026");
        QCOMPARE(portalRecords.size(), 4);

        // Verify period-aware book purchases loader for targeted portal reconciliation
        QList<Gstr2BookRecord> bookPurchases = Gstr2Reconciler::loadBookPurchasesForReconciliation(QDate(2026, 4, 1), QDate(2026, 4, 30), portalRecords);
        Gstr2ReconciliationSummary realSummary = Gstr2Reconciler::reconcile(bookPurchases, portalRecords, 1.0, 30);

        // Classic Battery Shoppe 2 invoices matched
        QVERIFY(realSummary.matchedCount >= 2);
        // Netplus and Canara Bank not in books
        QCOMPARE(realSummary.notInBooksCount, 2);
    }
}

void TestEnginesSuite::testGstr3BEngineCalculationAndExcelExport() {
    // Generate GSTR-3B for Mahadev Rice Industry for April 2026
    Gstr3BReturnSummary summary = Gstr3BEngine::generateFromDatabase(
        "06ABKFM5928Q1ZG",
        "MAHADEV RICE INDUSTRY",
        "06",
        QDate(2026, 4, 1),
        QDate(2026, 4, 30)
    );

    // Verify exact match with official Government GSTR-3B PDF (GSTR3B_06ABKFM5928Q1ZG_042026.pdf)
    // Table 3.1(c) Other outward supplies (Nil rated, exempted) = ₹10,18,86,696.00
    QCOMPARE(summary.table31.txValC, 101886696.00);
    QCOMPARE(summary.table31.txValA, 0.00);
    QCOMPARE(summary.table31.iAmtA, 0.00);
    QCOMPARE(summary.table31.cAmtA, 0.00);
    QCOMPARE(summary.table31.sAmtA, 0.00);

    // Table 5 Values of exempt inward supplies (Intra-State) = ₹15,93,85,347.00
    QCOMPARE(summary.table5.intraExempt, 159385347.00);
    QCOMPARE(summary.table5.interExempt, 0.00);

    // Table 4 Eligible ITC = ₹0.00 (all purchases exempt)
    QCOMPARE(summary.table4.netIgst, 0.00);
    QCOMPARE(summary.table4.netCgst, 0.00);
    QCOMPARE(summary.table4.netSgst, 0.00);
    QCOMPARE(summary.netPayableTotal, 0.00);

    // Test exporting to Excel template
    QString exportPath = "build/test_gstr3b_042026.xls";
    bool exported = Gstr3BEngine::exportToExcelTemplate(summary, exportPath, "gst-data/GSTR-3B.xls");
    QVERIFY(exported);
    QVERIFY(QFile::exists(exportPath));
}

void TestEnginesSuite::testTdsFvuExporter() {
    TdsDeductorInfo ded;
    ded.tan = "RTKM01234A";
    ded.pan = "ABKFM5928Q";
    ded.deductorName = "M/S MAHADEV RICE INDUSTRY";
    ded.deductorType = "O";
    ded.address1 = "Mandi Road";
    ded.city = "Hansi";
    ded.stateCode = "06";
    ded.pinCode = "125033";
    ded.email = "mahadev@example.com";
    ded.phone = "9812000000";
    ded.respPersonName = "Sushil Kumar";
    ded.respPersonDesig = "Partner";
    ded.respPersonPan = "ABKFM5928Q";

    QList<TdsChallanEntry> challans;
    TdsChallanEntry ch1;
    ch1.challanRecordNo = 1;
    ch1.bsrCode = "0210001";
    ch1.challanDate = "2026-05-07";
    ch1.challanNo = "00142";
    ch1.minorHead = "200";
    ch1.basicTax = 5000.00;
    ch1.totalChallanAmt = 5000.00;
    challans.append(ch1);

    QList<TdsDeducteeEntry> deductees;
    TdsDeducteeEntry dd1;
    dd1.deducteeRecordNo = 1;
    dd1.linkedChallanRecordNo = 1;
    dd1.deducteeCode = "02";
    dd1.pan = "AAAPA1234K";
    dd1.deducteeName = "Ramesh Kumar Contractor";
    dd1.sectionCode = "194C";
    dd1.paymentDate = "2026-04-30";
    dd1.amountPaid = 500000.00;
    dd1.tdsRate = 1.0;
    dd1.totalTaxDeposited = 5000.00;
    dd1.dateOfDeduction = "2026-04-30";
    deductees.append(dd1);

    QString fvuText = TdsFvuExporter::generateForm26Q("FY 2026-27", "Q1", ded, challans, deductees);
    QVERIFY(!fvuText.isEmpty());
    QVERIFY(fvuText.contains("^FH^"));
    QVERIFY(fvuText.contains("^BH^"));
    QVERIFY(fvuText.contains("^CD^"));
    QVERIFY(fvuText.contains("^DD^"));
    QVERIFY(fvuText.contains("RTKM01234A"));
    QVERIFY(fvuText.contains("26Q"));
}

void TestEnginesSuite::testGstr9AnnualReturnEngine() {
    Gstr9AnnualSummary s = Gstr9Engine::computeAnnualReturn("FY 2025-26");
    QCOMPARE(s.financialYear, "FY 2025-26");

    QString json = Gstr9Engine::exportGstr9Json(s);
    QVERIFY(!json.isEmpty());
    QVERIFY(json.contains("table4_outward_taxable"));
    QVERIFY(json.contains("table6_itc_availed"));

    QString csv = Gstr9Engine::exportGstr9Csv(s);
    QVERIFY(!csv.isEmpty());
    QVERIFY(csv.contains("4A,Supplies to Registered Persons (B2B)"));
}

void TestEnginesSuite::testBusyDataMigratorInspectionAndMigration() {
    BusyDataMigrator migrator;

    // Test directory inspection of busy-data
    QVariantMap inspDir = migrator.inspect_busy_data("busy-data");
    QVERIFY(inspDir.value("valid").toBool());
    QCOMPARE(inspDir.value("sourceType").toString(), "Busy");
    QCOMPARE(inspDir.value("companyName").toString(), "Mahadev Busy Test");
    QVERIFY(inspDir.value("accountsCount").toInt() >= 50);
    QVERIFY(inspDir.value("groupsCount").toInt() >= 20);
    QVERIFY(inspDir.value("itemsCount").toInt() >= 2);
    QVERIFY(inspDir.value("unitsCount").toInt() >= 4);
    QVERIFY(inspDir.value("isBalanced").toBool());
    QVERIFY(inspDir.value("glDiscrepancy").toDouble() < 0.01);

    // Test direct .bds file inspection
    QVariantMap inspFile = migrator.inspect_busy_data("busy-data/DATA/db12026.bds");
    QVERIFY(inspFile.value("valid").toBool());
    QCOMPARE(inspFile.value("sourceType").toString(), "Busy");

    // Test full migration into test database environment
    // Use an isolated temporary SQLite DB for migration verification
    QString testDbPath = "build/test_busy_migration.db";
    if (QFile::exists(testDbPath)) {
        QFile::remove(testDbPath);
    }

    // Initialize isolated SQLite database
    DatabaseManager::instance().initDatabase(testDbPath);

    bool migrationSuccess = migrator.migrate_busy_data("busy-data");
    QVERIFY(migrationSuccess);

    // Verify imported masters and transactions in SQLite
    // 1. Verify Master Accounts and Groups imported
    int accCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM parties;").toInt();
    QVERIFY(accCount >= 50);

    int grpCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM account_groups;").toInt();
    QVERIFY(grpCount >= 20);

    // 2. Verify Stock Items imported
    int itmCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_items;").toInt();
    QVERIFY(itmCount >= 2);

    int unitCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_units;").toInt();
    QVERIFY(unitCount >= 4);

    // 3. Verify Double-Entry GL Invariance in target SQLite database: SUM(Dr) == SUM(Cr)
    QVariantList glRows = DatabaseManager::instance().executeQuery("SELECT SUM(CASE WHEN dr_cr='Dr' THEN amount ELSE 0 END) as tot_dr, SUM(CASE WHEN dr_cr='Cr' THEN amount ELSE 0 END) as tot_cr FROM transactions;");
    double totalDr = 0.0;
    double totalCr = 0.0;
    if (!glRows.isEmpty()) {
        totalDr = glRows.first().toMap().value("tot_dr").toDouble();
        totalCr = glRows.first().toMap().value("tot_cr").toDouble();
    }
    QVERIFY(std::abs(totalDr - totalCr) < 0.01);

    // Restore standard test database
    DatabaseManager::instance().initDatabase("data/mahadev_rice_industry_data_002.db");
}

QTEST_MAIN(TestEnginesSuite)
#include "test_engines.moc"

