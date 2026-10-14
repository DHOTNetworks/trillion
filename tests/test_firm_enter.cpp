#include <QTest>
#include <QApplication>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QSignalSpy>
#include "../src/widgets/account_search_box.h"
#include "../src/widgets/voucher_common.h"
#include "../src/database_manager.h"
#include "../src/engine/ledger_pipeline.h"

class TestFirmEnter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Init database in memory for testing
        DatabaseManager::instance().switchDatabase(":memory:");
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS parties ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "name TEXT NOT NULL,"
            "alias TEXT,"
            "group_id INTEGER DEFAULT 8,"
            "group_name TEXT DEFAULT 'Sundry Debtors',"
            "gstin TEXT,"
            "city TEXT,"
            "state TEXT DEFAULT 'Punjab',"
            "state_code TEXT DEFAULT '03',"
            "opening_balance REAL DEFAULT 1500.0"
            ");"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO parties (name, gstin, city, state, opening_balance) VALUES "
            "('Aggarwal Traders', '03AAAAA0000A1Z5', 'Ludhiana', 'Punjab', 50000.00),"
            "('Aggarwal Rice Mills', '03BBBBB0000B1Z6', 'Amritsar', 'Punjab', 12000.00),"
            "('Karan Enterprises', '03CCCCC0000C1Z7', 'Jalandhar', 'Punjab', 25000.00);"
        );
        LedgerPipeline::instance().invalidateCache();
    }

    void testFirmSelectionOnEnterWithGridNavigation() {
        AccountSearchBox searchBox;
        searchBox.show();
        searchBox.setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&searchBox));

        bool enterTriggered = false;
        VoucherCommon::installGridNavigation(&searchBox, nullptr, nullptr, [&enterTriggered]() {
            enterTriggered = true;
        });

        QSignalSpy partySpy(&searchBox, &AccountSearchBox::partySelected);
        QSignalSpy dataSpy(&searchBox, &AccountSearchBox::partyDataSelected);

        // Type "Aggarwal" to trigger popup
        QTest::keyClicks(&searchBox, "Aggarwal");
        QCOMPARE(searchBox.isPopupVisible(), true);

        // Press Return key
        enterTriggered = false;
        QTest::keyClick(&searchBox, Qt::Key_Return);

        // Firm must be selected (Row 0 is "Aggarwal Traders")
        QCOMPARE(partySpy.count(), 1);
        QCOMPARE(dataSpy.count(), 1);
        QCOMPARE(searchBox.currentPartyName(), QString("Aggarwal Traders"));
        QVERIFY(searchBox.selectedPartyId() > 0);
        QCOMPARE(dataSpy.first().at(0).toMap().value("gstin").toString(), QString("03AAAAA0000A1Z5"));

        // Navigation onEnter must have fired
        QCOMPARE(enterTriggered, true);

        // Popup must now be closed
        QCOMPARE(searchBox.isPopupVisible(), false);
    }

    void testFirmSelectionWithDownArrowThenEnter() {
        AccountSearchBox searchBox;
        searchBox.show();
        searchBox.setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&searchBox));

        bool enterTriggered = false;
        VoucherCommon::installGridNavigation(&searchBox, nullptr, nullptr, [&enterTriggered]() {
            enterTriggered = true;
        });

        // Type "Aggarwal"
        QTest::keyClicks(&searchBox, "Aggarwal");
        QCOMPARE(searchBox.isPopupVisible(), true);

        // Down arrow to second item ("Aggarwal Rice Mills")
        QTest::keyClick(&searchBox, Qt::Key_Down);

        // Press Enter
        QTest::keyClick(&searchBox, Qt::Key_Return);

        QCOMPARE(searchBox.currentPartyName(), QString("Aggarwal Rice Mills"));
        QVERIFY(searchBox.selectedPartyId() > 0);
        QCOMPARE(enterTriggered, true);
    }

    void testFirmResolutionWhenPopupClosed() {
        AccountSearchBox searchBox;
        searchBox.show();
        searchBox.setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&searchBox));

        bool enterTriggered = false;
        VoucherCommon::installGridNavigation(&searchBox, nullptr, nullptr, [&enterTriggered]() {
            enterTriggered = true;
        });

        searchBox.setText("Karan Enterprises");
        searchBox.closeSearchPopup();

        QTest::keyClick(&searchBox, Qt::Key_Return);

        QCOMPARE(searchBox.currentPartyName(), QString("Karan Enterprises"));
        QVERIFY(searchBox.selectedPartyId() > 0);
        QCOMPARE(enterTriggered, true);
    }

    void testWordRankingFirstAndSecondWord() {
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO parties (name, gstin, city, state, opening_balance) VALUES "
            "('Sushil Karan Company', '03DDDDD0000D1Z8', 'Sirsa', 'Haryana', 10000.00),"
            "('Karan Kamboj', '03EEEEE0000E1Z9', 'Karnal', 'Haryana', 20000.00),"
            "('M/S Jai Karan Agro', '03FFFFF0000F1Z0', 'Hisar', 'Haryana', 30000.00),"
            "('Akarana Stores', '03GGGGG0000G1Z1', 'Rohtak', 'Haryana', 40000.00);"
        );
        LedgerPipeline::instance().invalidateCache();

        QVariantList results = LedgerPipeline::instance().searchLedgers("Kara", "", 10);
        QVERIFY(results.size() >= 4);

        QStringList names;
        for (const auto& r : results) {
            names << r.toMap().value("name").toString();
        }

        // Karan Kamboj (1st word prefix) must precede Sushil Karan Company (2nd word prefix)
        int idxKaranKamboj = names.indexOf("Karan Kamboj");
        int idxSushilKaran = names.indexOf("Sushil Karan Company");
        int idxJaiKaran = names.indexOf("M/S Jai Karan Agro");
        int idxAkarana = names.indexOf("Akarana Stores");

        QVERIFY(idxKaranKamboj != -1);
        QVERIFY(idxSushilKaran != -1);
        QVERIFY(idxJaiKaran != -1);
        QVERIFY(idxAkarana != -1);

        QVERIFY(idxKaranKamboj < idxSushilKaran);
        QVERIFY(idxSushilKaran < idxJaiKaran);
        QVERIFY(idxJaiKaran < idxAkarana);

        // Also test AccountSearchBox selection on Enter: typing "Kara" and hitting Enter chooses Karan Kamboj first
        AccountSearchBox searchBox;
        searchBox.show();
        searchBox.setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&searchBox));

        bool enterTriggered = false;
        VoucherCommon::installGridNavigation(&searchBox, nullptr, nullptr, [&enterTriggered]() {
            enterTriggered = true;
        });

        QTest::keyClicks(&searchBox, "Kara");
        QCOMPARE(searchBox.isPopupVisible(), true);

        // Immediate Return key should select top-ranked item "Karan Kamboj"
        QTest::keyClick(&searchBox, Qt::Key_Return);
        QCOMPARE(searchBox.currentPartyName(), QString("Karan Kamboj"));
        QCOMPARE(enterTriggered, true);
    }

    void testVehicleNoRealtimeUppercaseTyping() {
        QLineEdit edit;
        VoucherCommon::setupUppercaseInput(&edit);

        // 1. Entering lowercase vehicle number automatically transforms to UPPERCASE while typing
        QTest::keyClicks(&edit, "hr24ah9191");
        QCOMPARE(edit.text(), QString("HR24AH9191"));

        // 2. Typing in the middle preserves cursor position and uppercases characters
        edit.setCursorPosition(4); // cursor between '4' and 'A'
        QTest::keyClicks(&edit, "zz");
        QCOMPARE(edit.text(), QString("HR24ZZAH9191"));
        QCOMPARE(edit.cursorPosition(), 6);

        // 3. Hyphenated and mixed-case vehicle number
        edit.clear();
        QTest::keyClicks(&edit, "pb-10-ab-1234");
        QCOMPARE(edit.text(), QString("PB-10-AB-1234"));
    }

    void testVehicleNoDatabaseUppercaseConsistency() {
        // Create tables to verify deterministic schema and uppercase updates
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS gate_register ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "gate_pass_no TEXT,"
            "vehicle_no TEXT"
            ");"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO gate_register (gate_pass_no, entry_date, party_name, vehicle_no) VALUES "
            "('GP-01', '2026-10-12', 'Karan Traders', 'hr24ah9191'), ('GP-02', '2026-10-12', 'Karan Traders', 'pb10ab1234');"
        );

        // Run the deterministic uppercase normalization
        DatabaseManager::instance().executeNonQuery(
            "UPDATE gate_register SET vehicle_no = UPPER(TRIM(vehicle_no)) WHERE vehicle_no IS NOT NULL AND vehicle_no != UPPER(TRIM(vehicle_no));"
        );

        QVariant veh1 = DatabaseManager::instance().executeScalar("SELECT vehicle_no FROM gate_register WHERE gate_pass_no = 'GP-01';");
        QVariant veh2 = DatabaseManager::instance().executeScalar("SELECT vehicle_no FROM gate_register WHERE gate_pass_no = 'GP-02';");

        QCOMPARE(veh1.toString(), QString("HR24AH9191"));
        QCOMPARE(veh2.toString(), QString("PB10AB1234"));
    }

    void testBrokerSearchRankingAndConfiguration() {
        // Prepare broker database tables in memory
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS account_groups (id INTEGER PRIMARY KEY, name TEXT, code1st INTEGER);"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT OR REPLACE INTO account_groups (id, name, code1st) VALUES (143, 'Broker', 46);"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS vouchers (id INTEGER PRIMARY KEY, voucher_no TEXT, broker_name TEXT);"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS sales_invoices (id INTEGER PRIMARY KEY, broker_name TEXT, transport TEXT);"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS purchase_invoices (id INTEGER PRIMARY KEY, broker_name TEXT, transport TEXT);"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS sauda_contracts (id INTEGER PRIMARY KEY, broker_name TEXT, broker_id INTEGER);"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS dalali_settlements (id INTEGER PRIMARY KEY, broker_name TEXT, broker_id INTEGER);"
        );

        // Insert brokers with prefix vs word match vs substring match
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO parties (name, group_id, city) VALUES ('Karan Kamboj Broker', 143, 'Sirsa');"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO vouchers (voucher_no, voucher_date, voucher_type, account_type, party_name, amount, broker_name) VALUES ('V-1', '2026-10-12', 'Journal', 'Journal', 'P1', 100, 'Sushil Karan Dalal');"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO sales_invoices (invoice_no, invoice_date, customer_name, item_name, broker_name) VALUES ('INV-1', '2026-10-12', 'C1', 'Item1', 'Shree Balaji Karansingh');"
        );

        // 1. Static searchBrokers ranking test
        QVariantList brokers = AccountSearchBox::searchBrokers("Kara", 10);
        QVERIFY(brokers.size() >= 3);
        // Prefix match ("Karan Kamboj Broker") MUST be first
        QCOMPARE(brokers[0].toMap().value("name").toString(), QString("Karan Kamboj Broker"));
        // Second word match ("Sushil Karan Dalal") MUST be second
        QCOMPARE(brokers[1].toMap().value("name").toString(), QString("Sushil Karan Dalal"));
        // Substring inside word ("Shree Balaji Karansingh") MUST be third
        QCOMPARE(brokers[2].toMap().value("name").toString(), QString("Shree Balaji Karansingh"));

        // 2. AccountSearchBox widget configured for broker search
        QWidget brokerWin;
        auto* bLay = new QVBoxLayout(&brokerWin);
        auto* brokerBox = new AccountSearchBox(&brokerWin);
        auto* nextField = new QLineEdit(&brokerWin);
        bLay->addWidget(brokerBox);
        bLay->addWidget(nextField);
        brokerBox->configureForBrokerSearch();

        brokerWin.show();
        brokerBox->setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&brokerWin));

        bool advanced = false;
        VoucherCommon::installGridNavigation(brokerBox, nullptr, nextField, [&advanced, nextField]() {
            advanced = true;
            nextField->setFocus();
        });

        brokerBox->setFocus();
        QTest::keyClicks(brokerBox, "Kara");
        QVERIFY(brokerBox->isPopupVisible());

        // Press Enter to select the top-ranked item ("Karan Kamboj Broker") and navigate to nextField
        QTest::keyClick(brokerBox, Qt::Key_Return);
        QCOMPARE(brokerBox->text(), QString("Karan Kamboj Broker"));
        QVERIFY(!brokerBox->isPopupVisible());
        QVERIFY(advanced);
        QVERIFY(nextField->hasFocus());
    }

    void testTransportSearchRankingAndConfiguration() {
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS transport_dispatches ("
            "id INTEGER PRIMARY KEY, voucher_no TEXT, invoice_no TEXT, transporter_name TEXT, transporter_gstin TEXT, vehicle_no TEXT"
            ");"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS gate_register (id INTEGER PRIMARY KEY, transporter_name TEXT);"
        );

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO parties (name, group_id, city) VALUES ('Gagneja Freight Carriers Sirsa', 8, 'Sirsa');"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO transport_dispatches (dispatch_date, vehicle_no, party_name, transporter_name) VALUES ('2026-10-12', 'PB10', 'Party1', 'New Balaji Gagneja Transport');"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO sales_invoices (invoice_no, invoice_date, customer_name, item_name, transport) VALUES ('INV-2', '2026-10-12', 'C1', 'Item1', 'Shree Ram Gagnejawala Roadlines');"
        );

        // 1. Static searchTransporters ranking test
        QVariantList transports = AccountSearchBox::searchTransporters("Gagn", 10);
        QVERIFY(transports.size() >= 3);
        // Prefix match must be first
        QCOMPARE(transports[0].toMap().value("name").toString(), QString("Gagneja Freight Carriers Sirsa"));
        // Second word match must be second
        QCOMPARE(transports[1].toMap().value("name").toString(), QString("New Balaji Gagneja Transport"));
        // Substring inside word must be third
        QCOMPARE(transports[2].toMap().value("name").toString(), QString("Shree Ram Gagnejawala Roadlines"));

        // 2. AccountSearchBox widget configured for transport search
        QWidget transportWin;
        auto* tLay = new QVBoxLayout(&transportWin);
        auto* transportBox = new AccountSearchBox(&transportWin);
        auto* nextField2 = new QLineEdit(&transportWin);
        tLay->addWidget(transportBox);
        tLay->addWidget(nextField2);
        transportBox->configureForTransportSearch();

        transportWin.show();
        transportBox->setFocus();
        QVERIFY(QTest::qWaitForWindowExposed(&transportWin));

        bool advanced2 = false;
        VoucherCommon::installGridNavigation(transportBox, nullptr, nextField2, [&advanced2, nextField2]() {
            advanced2 = true;
            nextField2->setFocus();
        });

        transportBox->setFocus();
        QTest::keyClicks(transportBox, "Gagn");
        QVERIFY(transportBox->isPopupVisible());

        // Press Enter to select "Gagneja Freight Carriers Sirsa" and advance focus
        QTest::keyClick(transportBox, Qt::Key_Return);
        QCOMPARE(transportBox->text(), QString("Gagneja Freight Carriers Sirsa"));
        QVERIFY(!transportBox->isPopupVisible());
        QVERIFY(advanced2);
        QVERIFY(nextField2->hasFocus());
    }

    void testDatabaseMigrationAndBackfillOfTransportAndBroker() {
        // Setup schema with empty transport/broker in invoice, but populated in dispatches & vouchers
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS test_pi ("
            "id INTEGER PRIMARY KEY, voucher_no TEXT, invoice_no TEXT, transport TEXT, broker_name TEXT"
            ");"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS test_td ("
            "id INTEGER PRIMARY KEY, voucher_no TEXT, invoice_no TEXT, transporter_name TEXT"
            ");"
        );
        DatabaseManager::instance().executeNonQuery(
            "CREATE TABLE IF NOT EXISTS test_vch ("
            "id INTEGER PRIMARY KEY, voucher_no TEXT, instrument_no TEXT, broker_name TEXT"
            ");"
        );

        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO test_pi (id, voucher_no, invoice_no, transport, broker_name) VALUES "
            "(1, '2425/Purc-50', 'INV-50', '', ''), (2, '2425/Purc-51', 'INV-51', NULL, NULL);"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO test_td (voucher_no, invoice_no, transporter_name) VALUES "
            "('2425/Purc-50', 'INV-50', 'Sagar Road Lines'), ('2425/Purc-51', 'INV-51', 'A.T.C Transport Co.');"
        );
        DatabaseManager::instance().executeNonQuery(
            "INSERT INTO test_vch (voucher_no, instrument_no, broker_name) VALUES "
            "('2425/Purc-50', 'INV-50', 'Naveen Fazilka'), ('2425/Purc-51', 'INV-51', 'Krishana Thakur');"
        );

        // Run backfill updates identical to DatabaseManager::initialize()
        DatabaseManager::instance().executeNonQuery(
            "UPDATE test_pi SET transport = ("
            "  SELECT td.transporter_name FROM test_td td "
            "  WHERE (td.invoice_no = test_pi.invoice_no OR td.voucher_no = test_pi.voucher_no) "
            "    AND td.transporter_name IS NOT NULL AND TRIM(td.transporter_name) != '' LIMIT 1"
            ") WHERE (transport IS NULL OR TRIM(transport) = '') AND EXISTS ("
            "  SELECT 1 FROM test_td td "
            "  WHERE (td.invoice_no = test_pi.invoice_no OR td.voucher_no = test_pi.voucher_no) "
            "    AND td.transporter_name IS NOT NULL AND TRIM(td.transporter_name) != ''"
            ");"
        );
        DatabaseManager::instance().executeNonQuery(
            "UPDATE test_pi SET broker_name = ("
            "  SELECT v.broker_name FROM test_vch v "
            "  WHERE (v.voucher_no = test_pi.voucher_no OR v.instrument_no = test_pi.invoice_no) "
            "    AND v.broker_name IS NOT NULL AND TRIM(v.broker_name) != '' LIMIT 1"
            ") WHERE (broker_name IS NULL OR TRIM(broker_name) = '') AND EXISTS ("
            "  SELECT 1 FROM test_vch v "
            "  WHERE (v.voucher_no = test_pi.voucher_no OR v.instrument_no = test_pi.invoice_no) "
            "    AND v.broker_name IS NOT NULL AND TRIM(v.broker_name) != ''"
            ");"
        );

        QVariant tr1 = DatabaseManager::instance().executeScalar("SELECT transport FROM test_pi WHERE id = 1;");
        QVariant br1 = DatabaseManager::instance().executeScalar("SELECT broker_name FROM test_pi WHERE id = 1;");
        QVariant tr2 = DatabaseManager::instance().executeScalar("SELECT transport FROM test_pi WHERE id = 2;");
        QVariant br2 = DatabaseManager::instance().executeScalar("SELECT broker_name FROM test_pi WHERE id = 2;");

        QCOMPARE(tr1.toString(), QString("Sagar Road Lines"));
        QCOMPARE(br1.toString(), QString("Naveen Fazilka"));
        QCOMPARE(tr2.toString(), QString("A.T.C Transport Co."));
        QCOMPARE(br2.toString(), QString("Krishana Thakur"));
    }
};

QTEST_MAIN(TestFirmEnter)
#include "test_firm_enter.moc"
