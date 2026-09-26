#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>
#include <QStringList>

class PrintExportController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString lastExportedFile READ lastExportedFile NOTIFY lastExportedFileChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool isSuccess READ isSuccess NOTIFY statusMessageChanged)

public:
    explicit PrintExportController(QObject* parent = nullptr);

    QString lastExportedFile() const { return m_lastExportedFile; }
    QString statusMessage() const { return m_statusMessage; }
    bool isSuccess() const { return m_isSuccess; }

    // ==========================================
    // 1. GENERIC EXPORT ENGINES
    // ==========================================
    Q_INVOKABLE QString exportHtmlToPdf(const QString& htmlContent, const QString& defaultFileName, const QString& customPath = "", bool landscape = false);
    Q_INVOKABLE QString exportHtmlToOdf(const QString& htmlContent, const QString& defaultFileName, const QString& customPath = "", bool landscape = false);
    Q_INVOKABLE QString exportHtmlToExcel(const QString& htmlContent, const QString& defaultFileName, const QString& customPath = "");
    Q_INVOKABLE QString exportTableToCsv(const QStringList& headers, const QVector<QStringList>& rows, const QString& defaultFileName, const QString& customPath = "");
    Q_INVOKABLE bool printHtml(const QString& htmlContent, const QString& docTitle, bool landscape = false);

    // ==========================================
    // 2. VOUCHER EXPORT & PRINT
    // ==========================================
    // Sales Invoice
    Q_INVOKABLE QString export_sales_invoice_pdf(const QString& invoiceNo, const QString& customPath = "", const QString& copyType = "ORIGINAL FOR RECIPIENT");
    Q_INVOKABLE QString export_sales_invoice_duplicate_pdf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_sales_invoice_all_copies_pdf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_sales_invoice_odf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_sales_invoice_excel(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE bool print_sales_invoice(const QString& invoiceNo, const QString& copyType = "ORIGINAL FOR RECIPIENT");
    Q_INVOKABLE bool print_sales_invoice_duplicate(const QString& invoiceNo);
    Q_INVOKABLE bool print_sales_invoice_all_copies(const QString& invoiceNo);

    // Purchase Invoice
    Q_INVOKABLE QString export_purchase_invoice_pdf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_invoice_odf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_invoice_excel(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE bool print_purchase_invoice(const QString& invoiceNo);

    // Cash Voucher (Payment / Receipt)
    Q_INVOKABLE QString export_cash_voucher_pdf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_cash_voucher_odf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_cash_voucher_excel(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE bool print_cash_voucher(const QString& voucherNo);

    // Bank / Cheque Voucher (Payment / Receipt)
    Q_INVOKABLE QString export_cheque_voucher_pdf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_cheque_voucher_odf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_cheque_voucher_excel(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE bool print_cheque_voucher(const QString& voucherNo);

    // Journal Voucher
    Q_INVOKABLE QString export_journal_voucher_pdf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_journal_voucher_odf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_journal_voucher_excel(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE bool print_journal_voucher(const QString& voucherNo);

    // J-Form (Paddy Procurement)
    Q_INVOKABLE QString export_jform_voucher_pdf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_jform_voucher_odf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_jform_voucher_excel(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE bool print_jform_voucher(const QString& voucherNo);

    // I-Form (Commission Sale)
    Q_INVOKABLE QString export_iform_voucher_pdf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_iform_voucher_odf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_iform_voucher_excel(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE bool print_iform_voucher(const QString& voucherNo);

    // TDS / TCS / Tax Challan
    Q_INVOKABLE QString export_tds_voucher_pdf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_tds_voucher_odf(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE QString export_tds_voucher_excel(const QString& voucherNo, const QString& customPath = "");
    Q_INVOKABLE bool print_tds_voucher(const QString& voucherNo);

    // Debit / Credit Note
    Q_INVOKABLE QString export_debit_credit_note_pdf(const QString& noteNo, const QString& customPath = "");
    Q_INVOKABLE QString export_debit_credit_note_odf(const QString& noteNo, const QString& customPath = "");
    Q_INVOKABLE QString export_debit_credit_note_excel(const QString& noteNo, const QString& customPath = "");
    Q_INVOKABLE bool print_debit_credit_note(const QString& noteNo);

    // Transport Dispatch / Gate Pass
    Q_INVOKABLE QString export_transport_dispatch_pdf(int dispatchId, const QString& customPath = "");
    Q_INVOKABLE QString export_transport_dispatch_odf(int dispatchId, const QString& customPath = "");
    Q_INVOKABLE QString export_transport_dispatch_excel(int dispatchId, const QString& customPath = "");
    Q_INVOKABLE bool print_transport_dispatch(int dispatchId);

    // ==========================================
    // 3. STATEMENTS & FINANCIAL REPORTS
    // ==========================================
    // Ledger Statement (T-Account)
    Q_INVOKABLE QString export_ledger_statement_pdf(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_ledger_statement_odf(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_ledger_statement_excel(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_ledger_csv(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_ledger_statement(const QString& partyName, const QString& fromDate = "", const QString& toDate = "");

    // Day Book
    Q_INVOKABLE QString export_day_book_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_day_book_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_day_book_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_day_book(const QString& fromDate = "", const QString& toDate = "");

    // Trial Balance
    Q_INVOKABLE QString export_trial_balance_pdf(const QString& asOnDate = "", int mode = 0, const QString& customPath = "");
    Q_INVOKABLE QString export_trial_balance_odf(const QString& asOnDate = "", int mode = 0, const QString& customPath = "");
    Q_INVOKABLE QString export_trial_balance_excel(const QString& asOnDate = "", int mode = 0, const QString& customPath = "");
    Q_INVOKABLE bool print_trial_balance(const QString& asOnDate = "", int mode = 0);

    // Profit & Loss Statement
    Q_INVOKABLE QString export_profit_loss_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_profit_loss_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_profit_loss_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_profit_loss(const QString& fromDate = "", const QString& toDate = "");

    // Balance Sheet
    Q_INVOKABLE QString export_balance_sheet_pdf(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_balance_sheet_odf(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_balance_sheet_excel(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_balance_sheet(const QString& asOnDate = "");

    // Capital Accounts Schedule
    Q_INVOKABLE QString export_capital_accounts_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_capital_accounts_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_capital_accounts_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_capital_accounts(const QString& fromDate = "", const QString& toDate = "");

    // Cash & Bank Flow Statement
    Q_INVOKABLE QString export_cash_bank_flow_pdf(int flowType = 2, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_cash_bank_flow_odf(int flowType = 2, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_cash_bank_flow_excel(int flowType = 2, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_cash_bank_flow(int flowType = 2, const QString& fromDate = "", const QString& toDate = "");

    // Interest Calculator / Aank Statement
    Q_INVOKABLE QString export_interest_statement_pdf(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", double annualRate = 12.0, const QString& customPath = "");
    Q_INVOKABLE QString export_interest_statement_odf(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", double annualRate = 12.0, const QString& customPath = "");
    Q_INVOKABLE QString export_interest_statement_excel(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", double annualRate = 12.0, const QString& customPath = "");
    Q_INVOKABLE bool print_interest_statement(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", double annualRate = 12.0);

    // Depreciation Chart
    Q_INVOKABLE QString export_depreciation_chart_pdf(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_depreciation_chart_odf(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_depreciation_chart_excel(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_depreciation_chart(const QString& asOnDate = "");

    // ==========================================
    // 4. REGISTERS & INVENTORY / COMPLIANCE
    // ==========================================
    // Sales Register
    Q_INVOKABLE QString export_sales_register_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_sales_register_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_sales_register_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_sales_register_csv(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_sales_register(const QString& fromDate = "", const QString& toDate = "");

    // Purchase Register
    Q_INVOKABLE QString export_purchase_register_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_register_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_register_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_register_csv(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_purchase_register(const QString& fromDate = "", const QString& toDate = "");

    // Stock Register & Detail
    Q_INVOKABLE QString export_stock_register_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_stock_register_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_stock_register_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_stock_csv(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_stock_register(const QString& fromDate = "", const QString& toDate = "");

    // Item Movement Analysis
    Q_INVOKABLE QString export_item_movement_pdf(int itemId, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_item_movement_odf(int itemId, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_item_movement_excel(int itemId, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_item_movement(int itemId, const QString& fromDate = "", const QString& toDate = "");

    // Physical / Closing Stock Valuation
    Q_INVOKABLE QString export_closing_stock_pdf(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_closing_stock_odf(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_closing_stock_excel(const QString& asOnDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_closing_stock(const QString& asOnDate = "");

    // Milling Production Statement
    Q_INVOKABLE QString export_milling_statement_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_milling_statement_odf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_milling_statement_excel(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_milling_statement(const QString& fromDate = "", const QString& toDate = "");

    // Mandi Reports (Form M, Form 9, RDF, Market Fee)
    Q_INVOKABLE QString export_mandi_report_pdf(const QString& reportType, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_mandi_report_odf(const QString& reportType, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_mandi_report_excel(const QString& reportType, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_mandi_report(const QString& reportType, const QString& fromDate = "", const QString& toDate = "");

    // GST Compliance (GSTR-1, GSTR-2, GSTR-3B)
    Q_INVOKABLE QString export_gstr_report_pdf(const QString& gstrType, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_gstr_report_odf(const QString& gstrType, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_gstr_report_excel(const QString& gstrType, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE bool print_gstr_report(const QString& gstrType, const QString& fromDate = "", const QString& toDate = "");

    // Utilities
    Q_INVOKABLE void open_file_in_os(const QString& filePath);
    Q_INVOKABLE QString get_default_reports_dir() const;

signals:
    void lastExportedFileChanged();
    void statusMessageChanged();
    void printCompleted(bool success, const QString& message);

public:
    // HTML Renderers
    QString renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType = "ORIGINAL FOR RECIPIENT");
    QString renderSalesInvoiceSingleHtml(const QString& invoiceNo, const QString& copySubtitle);
    QString renderPurchaseInvoiceHtml(const QString& invoiceNo);
    QString renderCashVoucherHtml(const QString& voucherNo, const QString& voucherType = "Cash Payment");
    QString renderChequeVoucherHtml(const QString& voucherNo, const QString& voucherType = "Cheque Payment");
    QString renderJournalVoucherHtml(const QString& voucherNo);
    QString renderJFormVoucherHtml(const QString& voucherNo);
    QString renderIFormVoucherHtml(const QString& voucherNo);
    QString renderTdsVoucherHtml(const QString& voucherNo);
    QString renderDebitCreditNoteHtml(const QString& noteNo);
    QString renderTransportDispatchHtml(int dispatchId);

    QString renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate);
    QString renderDayBookHtml(const QString& fromDate, const QString& toDate);
    QString renderTrialBalanceHtml(const QString& asOnDate, int mode = 0);
    QString renderProfitLossHtml(const QString& fromDate, const QString& toDate);
    QString renderBalanceSheetHtml(const QString& asOnDate);
    QString renderCapitalAccountsHtml(const QString& fromDate, const QString& toDate);
    QString renderCashBankFlowHtml(int flowType, const QString& fromDate, const QString& toDate);
    QString renderInterestCalculationHtml(const QString& partyName, const QString& fromDate, const QString& toDate, double annualRate);
    QString renderDepreciationChartHtml(const QString& asOnDate);

    QString renderSalesRegisterHtml(const QString& fromDate, const QString& toDate);
    QString renderPurchaseRegisterHtml(const QString& fromDate, const QString& toDate);
    QString renderStockRegisterHtml(const QString& fromDate, const QString& toDate);
    QString renderItemMovementHtml(int itemId, const QString& fromDate, const QString& toDate);
    QString renderClosingStockHtml(const QString& asOnDate);
    QString renderMillingStatementHtml(const QString& fromDate, const QString& toDate);
    QString renderMandiReportHtml(const QString& reportType, const QString& fromDate, const QString& toDate);
    QString renderGstrReportHtml(const QString& gstrType, const QString& fromDate, const QString& toDate);

private:
    QVariantMap getFirmProfile();
    QVariantMap getPartyProfile(int partyId = 0, const QString& partyName = "");
    QString numberToWords(long long n);
    QString amountInWords(double amount);

    QString m_lastExportedFile;
    QString m_statusMessage;
    bool m_isSuccess = false;
};
