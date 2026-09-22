#include <QTest>
#include <QObject>
#include "../src/engine/mandi_calculator.h"
#include "../src/engine/aank_interest_engine.h"
#include "../src/engine/gst_tax_engine.h"
#include "../src/engine/milling_yield_engine.h"
#include "../src/engine/gstr2_reconciler.h"

using namespace MahadevERP;

class TestEnginesSuite : public QObject {
    Q_OBJECT

private slots:
    void testMandiCalculatorMath();
    void testAankInterestEngineMath();
    void testGstTaxEnginePosAndTcs();
    void testMillingYieldEngine();
    void testGstr2Reconciliation4WayMatch();
};

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
}

QTEST_MAIN(TestEnginesSuite)
#include "test_engines.moc"
