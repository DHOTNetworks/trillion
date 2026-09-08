#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

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

    // --- PDF EXPORT METHODS ---
    Q_INVOKABLE QString export_sales_invoice_pdf(const QString& invoiceNo, const QString& customPath = "", const QString& copyType = "ORIGINAL FOR RECIPIENT");
    Q_INVOKABLE QString export_sales_invoice_duplicate_pdf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_sales_invoice_all_copies_pdf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_invoice_pdf(const QString& invoiceNo, const QString& customPath = "");
    Q_INVOKABLE QString export_ledger_statement_pdf(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_stock_register_pdf(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");

    // --- NATIVE PRINT DIALOG METHODS ---
    Q_INVOKABLE bool print_sales_invoice(const QString& invoiceNo, const QString& copyType = "ORIGINAL FOR RECIPIENT");
    Q_INVOKABLE bool print_sales_invoice_duplicate(const QString& invoiceNo);
    Q_INVOKABLE bool print_sales_invoice_all_copies(const QString& invoiceNo);
    Q_INVOKABLE bool print_purchase_invoice(const QString& invoiceNo);
    Q_INVOKABLE bool print_ledger_statement(const QString& partyName, const QString& fromDate = "", const QString& toDate = "");
    Q_INVOKABLE bool print_stock_register(const QString& fromDate = "", const QString& toDate = "");

    // --- CSV / EXCEL EXPORT METHODS ---
    Q_INVOKABLE QString export_ledger_csv(const QString& partyName, const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_stock_csv(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_sales_register_csv(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");
    Q_INVOKABLE QString export_purchase_register_csv(const QString& fromDate = "", const QString& toDate = "", const QString& customPath = "");

    // --- UTILITIES ---
    Q_INVOKABLE void open_file_in_os(const QString& filePath);
    Q_INVOKABLE QString get_default_reports_dir() const;

signals:
    Q_SIGNAL void lastExportedFileChanged();
    Q_SIGNAL void statusMessageChanged();
    Q_SIGNAL void printCompleted(bool success, const QString& message);

private:
    QString renderSalesInvoiceHtml(const QString& invoiceNo, const QString& copyType = "ORIGINAL FOR RECIPIENT");
    QString renderSalesInvoiceSingleHtml(const QString& invoiceNo, const QString& copySubtitle);
    QString renderPurchaseInvoiceHtml(const QString& invoiceNo);
    QString renderLedgerStatementHtml(const QString& partyName, const QString& fromDate, const QString& toDate);
    QString renderStockRegisterHtml(const QString& fromDate, const QString& toDate);

    bool printHtml(const QString& htmlContent, const QString& docTitle, bool landscape = false);
    QString exportHtmlToPdf(const QString& htmlContent, const QString& defaultFileName, const QString& customPath, bool landscape = false);

    QVariantMap getFirmProfile();
    QString numberToWords(long long n);
    QString amountInWords(double amount);

    QString m_lastExportedFile;
    QString m_statusMessage;
    bool m_isSuccess = false;
};
