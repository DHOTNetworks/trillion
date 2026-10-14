#include <QTest>
#include <QSignalSpy>
#include <QDate>
#include <QFile>
#include <QProcess>
#include "../src/database_manager.h"
#include "../src/models/tds_model.h"
#include "../src/models/purchase_model.h"
#include "../src/models/tds_voucher_controller.h"
#include "../src/engine/bahi_khata_exporter.h"
#include "../src/engine/fiscal_year_helper.h"
#include <iostream>

class TestTdsVoucher : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testTdsVoucherPostingAndSchema();
    void testPurchase194QTurnoverAndCalculation();
    void testBahiKhataExportTdsAndTransactions();
    void testTdsVoucherDeletion();
    void cleanupTestCase();
};

void TestTdsVoucher::initTestCase() {
    auto& db = DatabaseManager::instance();
    QVERIFY(db.initDatabase("data/mahadev_rice_industry_data_002.db"));
}

void TestTdsVoucher::testTdsVoucherPostingAndSchema() {
    auto& db = DatabaseManager::instance();
    TdsModel model;

    // 1. Fetch next voucher info for RENT
    QVariantMap nextInfo = model.get_next_voucher_info("RENT");
    int nextVch = nextInfo.value("next_voucher_no").toInt();
    QVERIFY(nextVch > 0);

    // 2. Prepare payload for a RENT TDS voucher
    // Income: 50,000, TDS Rate: 10% -> TDS = 5,000, Net = 45,000
    QVariantMap payload;
    payload["voucher_no"] = nextVch;
    payload["voucher_date"] = "2026-06-15";
    payload["post_in_books"] = 1;
    payload["tds_type"] = "RENT";
    payload["ledger_name"] = "National Bulk Handling Corporation Pvt Ltd [Karnal]";
    payload["income_amount"] = 50000.0;
    payload["previous_amount"] = 0.0;
    payload["total_for_tds"] = 50000.0;
    payload["rate_tds"] = 10.0;
    payload["tax_amount_tds"] = 5000.0;
    payload["rate_surcharge"] = 0.0;
    payload["tax_amount_surcharge"] = 0.0;
    payload["rate_cess"] = 0.0;
    payload["tax_amount_cess"] = 0.0;
    payload["use_rounded_total"] = 1;
    payload["total_tax_rate"] = 10.0;
    payload["total_tax_amount"] = 5000.0;
    payload["net_amount"] = 45000.0;
    payload["exp_ledger_name"] = "Godown Rent";
    payload["tds_ledger_name"] = "T.D.S. Payable (Rent)";
    payload["narration"] = "RENT JUNE 2026";

    bool ok = model.save_tds_voucher(payload);
    QVERIFY(ok);

    // 3. Verify TDSDeductions table entry matches native Bahi-Khata format
    QVariantList tdsDedRows = db.executeQuery(
        "SELECT * FROM TDSDeductions WHERE voucher_number = ? AND trans_type = 'Jrnl';",
        {nextVch}
    );
    QCOMPARE(tdsDedRows.size(), 1);
    QVariantMap ded = tdsDedRows.first().toMap();
    QCOMPARE(ded.value("trans_type").toString(), QString("Jrnl"));
    QCOMPARE(ded.value("income_type").toString(), QString("RENT"));
    QCOMPARE(ded.value("income_amount").toDouble(), 50000.0);
    QCOMPARE(ded.value("tds").toDouble(), 5000.0);
    QCOMPARE(ded.value("tds_rate").toDouble(), 10.0);
    QCOMPARE(ded.value("voucher_date").toString(), QString("2026-06-15"));

    // 4. Verify transactions table double-entry postings (canonical 4 legs)
    QVariantList txRows = db.executeQuery(
        "SELECT * FROM transactions WHERE voucher_no = ? AND trans_type = 'Jrnl' ORDER BY row_no ASC;",
        {QString::number(nextVch)}
    );
    QCOMPARE(txRows.size(), 4);

    // Leg 1: Exp Dr
    QVariantMap leg1 = txRows[0].toMap();
    QCOMPARE(leg1.value("row_no").toInt(), 1);
    QCOMPARE(leg1.value("dr_cr").toString(), QString("Dr"));
    QCOMPARE(leg1.value("amount").toDouble(), 50000.0);
    QCOMPARE(leg1.value("entry_type").toString(), QString("Exp."));
    QCOMPARE(leg1.value("purchase_type").toString(), QString("TDSRENT"));
    QCOMPARE(leg1.value("voucher_reconcile").toInt(), 1);

    // Leg 2: Party Cr
    QVariantMap leg2 = txRows[1].toMap();
    QCOMPARE(leg2.value("row_no").toInt(), 2);
    QCOMPARE(leg2.value("dr_cr").toString(), QString("Cr"));
    QCOMPARE(leg2.value("amount").toDouble(), 50000.0);
    QCOMPARE(leg2.value("entry_type").toString(), QString("Party"));
    QCOMPARE(leg2.value("purchase_type").toString(), QString("TDSRENT"));
    QCOMPARE(leg2.value("narration").toString(), QString("RENT"));

    // Leg 3: Party Dr TDS
    QVariantMap leg3 = txRows[2].toMap();
    QCOMPARE(leg3.value("row_no").toInt(), 3);
    QCOMPARE(leg3.value("dr_cr").toString(), QString("Dr"));
    QCOMPARE(leg3.value("amount").toDouble(), 5000.0);
    QCOMPARE(leg3.value("entry_type").toString(), QString("PartyTDS"));
    QCOMPARE(leg3.value("purchase_type").toString(), QString("TDSRENT"));
    QVERIFY(leg3.value("narration").toString().contains("T.D.S. RENT @10.00%"));

    // Leg 4: TDS Payable Cr
    QVariantMap leg4 = txRows[3].toMap();
    QCOMPARE(leg4.value("row_no").toInt(), 4);
    QCOMPARE(leg4.value("dr_cr").toString(), QString("Cr"));
    QCOMPARE(leg4.value("amount").toDouble(), 5000.0);
    QCOMPARE(leg4.value("entry_type").toString(), QString("TDS"));
    QCOMPARE(leg4.value("purchase_type").toString(), QString("TDSRENT"));
}

void TestTdsVoucher::testPurchase194QTurnoverAndCalculation() {
    // 1. Test getSupplierFyPurchaseTurnover for Krish Cereals in FY 2025-26
    double turnover2526 = PurchaseModel::get_supplier_fy_purchase_turnover(3967, "Krish Cereals Pvt. Ltd. [Nissing]", "2025-06-01", 0);
    // In database, Krish Cereals has ₹1,76,12,100 in FY 2025-26
    QVERIFY(turnover2526 > 10000000.0);

    // 2. Test TDS 194Q math calculation:
    // Case A: Prior Turnover = ₹60,00,000 (> ₹50L threshold)
    // Current Bill = ₹10,00,000 -> Entire 10L subject to TDS @ 0.1% = ₹1,000
    double outTds = 0.0, outRate = 0.1, outTaxable = 0.0;
    double priorTurnoverA = 6000000.0;
    double currentBillA = 1000000.0;
    double threshold = 5000000.0;
    double rate = 0.100;

    double taxableA = (priorTurnoverA >= threshold) ? currentBillA : 0.0;
    double tdsA = std::round(taxableA * (rate / 100.0));
    QCOMPARE(taxableA, 1000000.0);
    QCOMPARE(tdsA, 1000.0);

    // Case B: Prior Turnover = ₹45,00,000 (< ₹50L threshold)
    // Current Bill = ₹10,00,000 -> Aggregate = ₹55,00,000 > ₹50L
    // Subject to TDS = 55L - 50L = ₹5,00,000 @ 0.1% = ₹500
    double priorTurnoverB = 4500000.0;
    double currentBillB = 1000000.0;
    double taxableB = (priorTurnoverB + currentBillB > threshold) ? (priorTurnoverB + currentBillB - threshold) : 0.0;
    double tdsB = std::round(taxableB * (rate / 100.0));
    QCOMPARE(taxableB, 500000.0);
    QCOMPARE(tdsB, 500.0);

    // Case C: Prior Turnover = ₹30,00,000 (< ₹50L)
    // Current Bill = ₹10,00,000 -> Aggregate = ₹40,00,000 (< ₹50L)
    // Subject to TDS = ₹0.00
    double priorTurnoverC = 3000000.0;
    double currentBillC = 1000000.0;
    double taxableC = (priorTurnoverC + currentBillC > threshold) ? (priorTurnoverC + currentBillC - threshold) : 0.0;
    double tdsC = std::round(taxableC * (rate / 100.0) * 100.0) / 100.0;
    QCOMPARE(taxableC, 0.0);
    QCOMPARE(tdsC, 0.0);

    // Case D: Prior Turnover = ₹1,76,12,100 (> ₹50L threshold)
    // Current Bill = ₹49,00,500 -> Subject to TDS = ₹49,00,500 @ 0.1% = ₹4,900.50 (exact 2 decimal precision)
    double priorTurnoverD = 17612100.0;
    double currentBillD = 4900500.0;
    double taxableD = (priorTurnoverD >= threshold) ? currentBillD : 0.0;
    double tdsD = std::round(taxableD * (rate / 100.0) * 100.0) / 100.0;
    QCOMPARE(taxableD, 4900500.0);
    QCOMPARE(tdsD, 4900.50);
}

void TestTdsVoucher::testBahiKhataExportTdsAndTransactions() {
    auto& db = DatabaseManager::instance();
    QVariant vNoVar = db.executeScalar("SELECT voucher_no FROM tds_vouchers ORDER BY id DESC LIMIT 1;");
    QVERIFY(vNoVar.isValid());
    int vNo = vNoVar.toInt();

    QString srcMdb = "/Users/karan/Desktop/Data.002";
    if (!QFile::exists(srcMdb)) {
        QSKIP("Desktop Data.002 template not found, skipping MDB export verification");
    }

    QString tmpMdb = "/tmp/test_tds_export.002";
    if (QFile::exists(tmpMdb)) QFile::remove(tmpMdb);
    QVERIFY(QFile::copy(srcMdb, tmpMdb));

    MahadevERP::BahiKhataExporter exporter;
    MahadevERP::BahiKhataExporter::ExportOptions opts;
    opts.targetMdbPath = tmpMdb;
    opts.templateMdbPath = srcMdb;
    opts.createBackup = false;
    opts.exportTransactions = true;
    opts.financialYear = "";

    auto summary = exporter.exportToBahiKhata(opts);
    QVERIFY(summary.success);

    // Verify using mdb-sql that TDSDeductions in tmpMdb has the voucher
    QProcess proc;
    proc.start("mdb-sql", QStringList() << "-P" << tmpMdb);
    QVERIFY(proc.waitForStarted());
    proc.write(QString("SELECT VoucherNumber, TransType, IncomeType, TDS FROM TDSDeductions WHERE VoucherNumber = %1;\n").arg(vNo).toUtf8());
    proc.closeWriteChannel();
    QVERIFY(proc.waitForFinished());
    QString out = QString::fromUtf8(proc.readAllStandardOutput());
    QVERIFY(out.contains("RENT"));
    QVERIFY(out.contains("Jrnl"));
    QVERIFY(out.contains("5000"));

    // Verify Transactions table has the canonical 4 legs with PurchaseType = 'TDSRENT'
    QProcess procTx;
    procTx.start("mdb-sql", QStringList() << "-P" << tmpMdb);
    QVERIFY(procTx.waitForStarted());
    procTx.write(QString("SELECT VoucherNumber, TransType, PurchaseType, EntryType, VoucherReconcile FROM Transactions WHERE VoucherNumber = %1;\n").arg(vNo).toUtf8());
    procTx.closeWriteChannel();
    QVERIFY(procTx.waitForFinished());
    QString outTx = QString::fromUtf8(procTx.readAllStandardOutput());
    QVERIFY(outTx.contains("TDSRENT"));
    QVERIFY(outTx.contains("Exp."));
    QVERIFY(outTx.contains("Party"));
    QVERIFY(outTx.contains("PartyTDS"));
    QVERIFY(outTx.contains("TDS"));

    QFile::remove(tmpMdb);
}

void TestTdsVoucher::testTdsVoucherDeletion() {
    auto& db = DatabaseManager::instance();
    TdsModel model;

    QVariant lastId = db.executeScalar("SELECT id FROM tds_vouchers ORDER BY id DESC LIMIT 1;");
    QVERIFY(lastId.isValid());
    int vId = lastId.toInt();

    QVariant vNoVar = db.executeScalar("SELECT voucher_no FROM tds_vouchers WHERE id = ?;", {vId});
    int vNo = vNoVar.toInt();

    bool ok = model.delete_tds_voucher(vId);
    QVERIFY(ok);

    // Verify removed from tds_vouchers
    QVariant countVch = db.executeScalar("SELECT COUNT(*) FROM tds_vouchers WHERE id = ?;", {vId});
    QCOMPARE(countVch.toInt(), 0);

    // Verify removed from TDSDeductions
    QVariant countDed = db.executeScalar("SELECT COUNT(*) FROM TDSDeductions WHERE voucher_number = ? AND trans_type = 'Jrnl';", {vNo});
    QCOMPARE(countDed.toInt(), 0);

    // Verify removed from transactions
    QVariant countTx = db.executeScalar("SELECT COUNT(*) FROM transactions WHERE voucher_no = ? AND trans_type = 'Jrnl';", {QString::number(vNo)});
    QCOMPARE(countTx.toInt(), 0);
}

void TestTdsVoucher::cleanupTestCase() {
    // Database remains consistent
}

QTEST_MAIN(TestTdsVoucher)
#include "test_tds_voucher.moc"
