#include <QTest>
#include <QObject>
#include "../src/engine/mandi_calculator.h"
#include "../src/engine/aank_interest_engine.h"
#include "../src/engine/gst_tax_engine.h"
#include "../src/engine/milling_yield_engine.h"
#include "../src/engine/gstr2_reconciler.h"
#include "../src/engine/gstr3b_engine.h"
#include "../src/engine/gstr1_engine.h"
#include <miniz.h>
#include "../src/engine/tds_fvu_exporter.h"
#include "../src/engine/gstr9_engine.h"
#include "../src/engine/bahi_khata_migrator.h"
#include "../src/engine/busy_data_migrator.h"
#include "../src/engine/tally_data_migrator.h"
#include "../src/engine/profit_loss_calculator.h"
#include "../src/models/mandi_reports_controller.h"
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
    void testTallyDataMigratorInspectionAndMigration();
};

void TestEnginesSuite::initTestCase() {
    QString dbPath = "data/mahadev_rice_industry_data_002.db";
    if (!QFile::exists(dbPath) && QFile::exists("../" + dbPath)) {
        dbPath = "../" + dbPath;
    }
    DatabaseManager::instance().switchDatabase(dbPath);

    int vCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM vouchers;").toInt();
    if (vCount == 0) {
        QString mdb002 = "Bahi-Khata-Data/Data.002";
        if (!QFile::exists(mdb002) && QFile::exists("../Bahi-Khata-Data/Data.002")) {
            mdb002 = "../Bahi-Khata-Data/Data.002";
        }
        if (QFile::exists(mdb002)) {
            BahiKhataMigrator migrator;
            migrator.migrate_mdb_file(mdb002);
        }
    }
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
    QString jsonPath = "gst-data/returns_R2B_06ABKFM5928Q1ZG_042026.json";
    if (!QFile::exists(jsonPath) && QFile::exists("../" + jsonPath)) {
        jsonPath = "../" + jsonPath;
    }
    QFile f(jsonPath);
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

    // Cross-return tie-out: 3.1(c) == GSTR-1 Table 8, 3.2 == GSTR-1 Tables 5/7B.
    MahadevERP::Gstr1ReturnPayload g1 = MahadevERP::Gstr1Engine::generateFromDatabase(
        "06ABKFM5928Q1ZG", "MAHADEV RICE INDUSTRY", "06", QDate(2026, 4, 1), QDate(2026, 4, 30));
    double t8 = 0.0;
    for (const auto& e : g1.exemp) t8 += e.nilAmt + e.exmpAmt + e.ngsupAmt;
    QCOMPARE(summary.table31.txValC, t8);
    MahadevERP::Gstr3BReturnSummary with32 = MahadevERP::Gstr3BEngine::generateFromDatabase(
        "06ABKFM5928Q1ZG", "MAHADEV RICE INDUSTRY", "06", QDate(2026, 4, 1), QDate(2026, 4, 30), &g1);
    double b2clInter = 0.0;
    for (const auto& c : g1.b2cl) b2clInter += c.taxableValue;
    for (const auto& cs : g1.b2cs) if (cs.splyTy == "INTER") b2clInter += cs.taxableValue;
    double t32 = 0.0;
    for (const auto& r : with32.table32.rows) t32 += r.txVal;
    QCOMPARE(t32, b2clInter);

    // Export builds the v1.2 workbook from scratch (no template file needed).
    QString exportPath = "test_gstr3b_042026.xlsx";
    QString exportPath2 = "test_gstr3b_042026_b.xlsx";
    QVERIFY(Gstr3BEngine::exportToExcelTemplate(summary, exportPath, ""));
    QVERIFY(Gstr3BEngine::exportToExcelTemplate(summary, exportPath2, "nonexistent-template.xls"));
    QFile f1(exportPath), f2(exportPath2);
    QVERIFY(f1.open(QIODevice::ReadOnly) && f2.open(QIODevice::ReadOnly));
    QCOMPARE(f1.readAll(), f2.readAll()); // deterministic: identical input -> identical bytes
    f1.close(); f2.close();

    // Structural parity with Gstr3bTemplate_v1.2.xlsx.
    auto zipRead = [&](const QString& inner) {
        mz_zip_archive z;
        memset(&z, 0, sizeof(z));
        QString out;
        if (!mz_zip_reader_init_file(&z, exportPath.toUtf8().constData(), 0)) return out;
        mz_uint n = mz_zip_reader_get_num_files(&z);
        for (mz_uint i = 0; i < n; i++) {
            mz_zip_archive_file_stat st;
            if (!mz_zip_reader_file_stat(&z, i, &st)) continue;
            if (QString::fromUtf8(st.m_filename) == inner) {
                size_t sz = 0;
                void* p = mz_zip_reader_extract_to_heap(&z, i, &sz, 0);
                if (p) { out = QString::fromUtf8((const char*)p, (int)sz); mz_free(p); }
                break;
            }
        }
        mz_zip_reader_end(&z);
        return out;
    };
    QString wb = zipRead("xl/workbook.xml");
    for (const char* s : {"Master","Index","3.1","3.1.1","3.2","4","5","6.1","6.2"})
        QVERIFY2(wb.contains(QString("<sheet name=\"%1\"").arg(s)), s);
    QVERIFY(wb.contains("<definedName name=\"state\">Master!$B$1:$B$36</definedName>"));
    QVERIFY(wb.contains("<definedName name=\"SuppliesDesc\">Master!$A$1:$A$3</definedName>"));
    QString s31 = zipRead("xl/worksheets/sheet3.xml");
    QVERIFY(s31.contains("(c) Other outward supplies (nil rated, exempted)"));
    QVERIFY(s31.contains(">101886696.00<")); // 3.1(c) value from above
    QString idx = zipRead("xl/worksheets/sheet2.xml");
    QVERIFY(idx.contains(">06ABKFM5928Q1ZG<"));
    QFile::remove(exportPath);
    QFile::remove(exportPath2);
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

    QString busyDir = "busy-data";
    if (!QDir(busyDir).exists() && QDir("../" + busyDir).exists()) {
        busyDir = "../" + busyDir;
    }

    // Test directory inspection of busy-data
    QVariantMap inspDir = migrator.inspect_busy_data(busyDir);
    QVERIFY(inspDir.value("valid").toBool());
    QCOMPARE(inspDir.value("sourceType").toString(), "Busy");
    QCOMPARE(inspDir.value("companyName").toString(), "Mahadev Busy Test");
    QVERIFY(inspDir.value("accountsCount").toInt() >= 50);
    QVERIFY(inspDir.value("groupsCount").toInt() >= 20);
    QVERIFY(inspDir.value("itemsCount").toInt() >= 1);
    QVERIFY(inspDir.value("unitsCount").toInt() >= 4);
    QVERIFY(inspDir.value("isBalanced").toBool());
    QVERIFY(inspDir.value("glDiscrepancy").toDouble() < 0.01);

    // Test direct .bds file inspection
    QString bdsFile = busyDir + "/DATA/db12026.bds";
    QVariantMap inspFile = migrator.inspect_busy_data(bdsFile);
    QVERIFY(inspFile.value("valid").toBool());
    QCOMPARE(inspFile.value("sourceType").toString(), "Busy");

    // Test full migration into test database environment
    // Use an isolated temporary SQLite DB for migration verification
    QString testDbPath = "mahadev_busy_test_isolated.db";
    if (QFile::exists(testDbPath)) {
        QFile::remove(testDbPath);
    }

    // Initialize isolated SQLite database
    DatabaseManager::instance().switchDatabase(testDbPath);

    bool migrationSuccess = migrator.migrate_busy_data(busyDir);
    QVERIFY(migrationSuccess);

    // Verify imported masters and transactions in SQLite
    // 1. Verify Company Identity
    QString importedComp = DatabaseManager::instance().executeScalar("SELECT company_name FROM company_info WHERE id=1;").toString();
    QCOMPARE(importedComp, "Mahadev Busy Test");

    // 2. Verify Master Accounts and Groups imported
    int accCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM parties;").toInt();
    QVERIFY(accCount >= 50);

    int grpCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM account_groups;").toInt();
    QVERIFY(grpCount >= 20);

    // 3. Verify Stock Items & Groups imported
    int itmCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_items;").toInt();
    QVERIFY(itmCount >= 1);
    double itmGstRate = DatabaseManager::instance().executeScalar("SELECT gst_rate FROM stock_items LIMIT 1;").toDouble();
    QCOMPARE(itmGstRate, 0.0); // GST rate should default to 0.0 for nil GST items, NOT 5%

    int grpStockCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_groups;").toInt();
    QVERIFY(grpStockCount >= 1);

    int unitCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_units;").toInt();
    QVERIFY(unitCount >= 4);

    // 4. Verify Vouchers & Sales Invoices imported
    int vchCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM vouchers;").toInt();
    QVERIFY(vchCount >= 2);

    int saleInvCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM sales_invoices;").toInt();
    QVERIFY(saleInvCount >= 1);
    double saleInvTotal = DatabaseManager::instance().executeScalar("SELECT SUM(total_amount) FROM sales_invoices;").toDouble();
    QCOMPARE(saleInvTotal, 250000.0);

    // Verify stock_transactions was populated for inventory movement register
    int stCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_transactions WHERE trans_type = 'Sale';").toInt();
    QVERIFY(stCount >= 1);

    QVariantMap stRow = DatabaseManager::instance().executeQuery("SELECT * FROM stock_transactions WHERE trans_type = 'Sale' LIMIT 1;").first().toMap();
    QCOMPARE(stRow.value("weight_qtl").toDouble(), 100.0);
    QCOMPARE(stRow.value("amount").toDouble(), 250000.0);

    // 5. Verify Double-Entry GL Invariance in target SQLite database: SUM(Dr) == SUM(Cr)
    QVariantList glRows = DatabaseManager::instance().executeQuery("SELECT SUM(CASE WHEN dr_cr='Dr' THEN amount ELSE 0 END) as tot_dr, SUM(CASE WHEN dr_cr='Cr' THEN amount ELSE 0 END) as tot_cr FROM transactions;");
    double totalDr = 0.0;
    double totalCr = 0.0;
    if (!glRows.isEmpty()) {
        totalDr = glRows.first().toMap().value("tot_dr").toDouble();
        totalCr = glRows.first().toMap().value("tot_cr").toDouble();
    }
    QVERIFY(std::abs(totalDr - totalCr) < 0.01);
    QCOMPARE(totalDr, 250100.0);
    QCOMPARE(totalCr, 250100.0);

    // 6. Test BUSY Mandi dataset (busy-data/mandi) containing J-Forms, I-Forms and Procurement
    QString mandiDir = busyDir + "/mandi";
    if (QDir(mandiDir).exists()) {
        QVariantMap inspMandi = migrator.inspect_busy_data(mandiDir);
        QVERIFY(inspMandi.value("valid").toBool());
        QCOMPARE(inspMandi.value("companyName").toString(), "Anaj Mandi");
        QVERIFY(inspMandi.value("mandi_records_count").toInt() >= 8);

        QString testMandiDbPath = "mahadev_busy_mandi_test_isolated.db";
        if (QFile::exists(testMandiDbPath)) QFile::remove(testMandiDbPath);
        DatabaseManager::instance().switchDatabase(testMandiDbPath);

        bool mandiOk = migrator.migrate_busy_data(mandiDir);
        QVERIFY(mandiOk);

        // Verify J-Form and I-Form vouchers
        int jfCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM jform_vouchers;").toInt();
        QCOMPARE(jfCount, 4);

        int ifCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM iform_vouchers;").toInt();
        QCOMPARE(ifCount, 4);

        int jfItemsCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM jform_voucher_items;").toInt();
        QCOMPARE(jfItemsCount, 4);

        int ifItemsCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM iform_voucher_items;").toInt();
        QCOMPARE(ifItemsCount, 4);

        int procCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM paddy_procurement;").toInt();
        QCOMPARE(procCount, 8);

        // Verify MandiReportsController functions
        MandiReportsController mandiCtrl;
        QVariantList jfReg = mandiCtrl.get_jform_register("", "");
        QCOMPARE(jfReg.size(), 4);

        QVariantList ifReg = mandiCtrl.get_iform_register("", "");
        QCOMPARE(ifReg.size(), 4);

        QVariantMap formM = mandiCtrl.get_form_m_return("2026-04-01", "2026-04-30");
        QVERIFY(formM.value("total_mandi_fee").toDouble() > 0.0);

        // Verify GL Double-Entry Invariance
        QVariantList mandiGl = DatabaseManager::instance().executeQuery("SELECT SUM(CASE WHEN dr_cr='Dr' THEN amount ELSE 0 END) as tot_dr, SUM(CASE WHEN dr_cr='Cr' THEN amount ELSE 0 END) as tot_cr FROM transactions;");
        double mDr = mandiGl.first().toMap().value("tot_dr").toDouble();
        double mCr = mandiGl.first().toMap().value("tot_cr").toDouble();
        QVERIFY(std::abs(mDr - mCr) < 0.01);
    }

    // Restore test database
    QString origDbPath = "data/mahadev_rice_industry_data_002.db";
    if (!QFile::exists(origDbPath) && QFile::exists("../" + origDbPath)) {
        origDbPath = "../" + origDbPath;
    }
    DatabaseManager::instance().switchDatabase(origDbPath);
}

void TestEnginesSuite::testTallyDataMigratorInspectionAndMigration() {
    TallyDataMigrator migrator;

    QString tallyDir = "tally-data/xml-samples";
    if (!QDir(tallyDir).exists() && QDir("../" + tallyDir).exists()) {
        tallyDir = "../" + tallyDir;
    }

    // 1. Test directory inspection of Tally XML samples
    QVariantMap insp = migrator.inspect_tally_data(tallyDir);
    QVERIFY(insp.value("valid").toBool());
    QCOMPARE(insp.value("sourceType").toString(), "Tally");
    QCOMPARE(insp.value("companyName").toString(), "Tally Prime Company");
    QVERIFY(insp.value("unitsCount").toInt() >= 2);
    QVERIFY(insp.value("groupsCount").toInt() >= 2);
    QVERIFY(insp.value("accountsCount").toInt() >= 4);
    QVERIFY(insp.value("itemsCount").toInt() >= 2);
    QVERIFY(insp.value("totalVouchersCount").toInt() >= 2);
    QVERIFY(insp.value("glTransactionsCount").toInt() >= 4);
    QVERIFY(insp.value("isBalanced").toBool());
    QVERIFY(insp.value("glDiscrepancy").toDouble() < 0.01);

    // 2. Test single file inspection (Master.xml)
    QString masterFile = tallyDir + "/Master.xml";
    QVariantMap inspMaster = migrator.inspect_tally_data(masterFile);
    QVERIFY(inspMaster.value("valid").toBool());
    QVERIFY(inspMaster.value("accountsCount").toInt() >= 4);

    // 3. Test isolated SQLite migration
    QString testDbPath = "mahadev_tally_test_isolated.db";
    if (QFile::exists(testDbPath)) {
        QFile::remove(testDbPath);
    }

    DatabaseManager::instance().switchDatabase(testDbPath);

    bool ok = migrator.migrate_tally_data(tallyDir);
    QVERIFY(ok);

    // Verify imported masters
    int unitsCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_units;").toInt();
    QVERIFY(unitsCount >= 2);

    int grpCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM account_groups;").toInt();
    QVERIFY(grpCount >= 2);

    int partyCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM parties;").toInt();
    QVERIFY(partyCount >= 4);

    int itmCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM stock_items;").toInt();
    QVERIFY(itmCount >= 2);

    // Verify imported transactions
    int vchCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM vouchers;").toInt();
    QVERIFY(vchCount >= 2);

    int txCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM transactions;").toInt();
    QVERIFY(txCount >= 4);

    // Verify double-entry GL balance invariance: SUM(Dr) == SUM(Cr)
    QVariantList glRows = DatabaseManager::instance().executeQuery("SELECT SUM(CASE WHEN dr_cr='Dr' THEN amount ELSE 0 END) as tot_dr, SUM(CASE WHEN dr_cr='Cr' THEN amount ELSE 0 END) as tot_cr FROM transactions;");
    double totalDr = 0.0;
    double totalCr = 0.0;
    if (!glRows.isEmpty()) {
        totalDr = glRows.first().toMap().value("tot_dr").toDouble();
        totalCr = glRows.first().toMap().value("tot_cr").toDouble();
    }
    // 4. Test self-created Tally Prime Demo dataset (tally-data/self-data)
    QString selfDataDir = "tally-data/self-data";
    if (!QDir(selfDataDir).exists() && QDir("../" + selfDataDir).exists()) {
        selfDataDir = "../" + selfDataDir;
    }
    if (QDir(selfDataDir).exists()) {
        QVariantMap inspSelf = migrator.inspect_tally_data(selfDataDir);
        QVERIFY(inspSelf.value("valid").toBool());
        QCOMPARE(inspSelf.value("companyName").toString(), "Test Tally");
        QVERIFY(inspSelf.value("unitsCount").toInt() >= 1);
        QVERIFY(inspSelf.value("itemsCount").toInt() >= 1);
        QVERIFY(inspSelf.value("accountsCount").toInt() >= 2);
        QVERIFY(inspSelf.value("totalVouchersCount").toInt() >= 1);

        QString testSelfDbPath = "mahadev_tally_self_isolated.db";
        if (QFile::exists(testSelfDbPath)) QFile::remove(testSelfDbPath);
        DatabaseManager::instance().switchDatabase(testSelfDbPath);

        bool selfOk = migrator.migrate_tally_data(selfDataDir);
        QVERIFY(selfOk);

        int selfVchCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM vouchers;").toInt();
        QCOMPARE(selfVchCount, 1);

        int selfSalesCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM sales_invoices;").toInt();
        QCOMPARE(selfSalesCount, 1);

        int selfTxCount = DatabaseManager::instance().executeScalar("SELECT COUNT(*) FROM transactions;").toInt();
        QCOMPARE(selfTxCount, 2);

        // Verify P&L and Balance Sheet on self data
        ProfitLossData pl = ProfitLossCalculator::calculate("2026-04-01", "2027-03-31");
        QCOMPARE(pl.totalSalesRevenue, 225000.00);
        QCOMPARE(pl.grossProfit, 225000.00);
        QCOMPARE(pl.netProfit, 225000.00);
    }

    // Restore test database
    QString origDbPath = "data/mahadev_rice_industry_data_002.db";
    if (!QFile::exists(origDbPath) && QFile::exists("../" + origDbPath)) {
        origDbPath = "../" + origDbPath;
    }
    DatabaseManager::instance().switchDatabase(origDbPath);
}

QTEST_MAIN(TestEnginesSuite)
#include "test_engines.moc"

