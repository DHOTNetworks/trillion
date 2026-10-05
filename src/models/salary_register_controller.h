#pragma once

#include <QObject>
#include <QString>
#include <QDate>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

namespace MahadevERP {

struct EmployeeSalaryItem {
    int partyId = 0;
    int legacyCode = 0;
    QString employeeName;
    QString designation;
    QString phone;
    QString station;
    double masterSalary = 0.0;
    int daysInMonth = 30;
    double presentDays = 30.0;
    double earnedBasic = 0.0;
    double allowances = 0.0;
    double pfDeduction = 0.0;
    double tdsDeduction = 0.0;
    double advanceDeduction = 0.0;
    double otherDeduction = 0.0;
    double netPayable = 0.0;
    bool isSelected = true;
    bool isPosted = false;
    QString postedVoucherNo;
    QString postedVoucherDate;
    int salaryPaymentId = 0;
};

struct SalaryRegisterSummary {
    int totalEmployees = 0;
    int selectedEmployees = 0;
    int postedEmployees = 0;
    int pendingEmployees = 0;
    double totalMasterSalary = 0.0;
    double totalEarnedBasic = 0.0;
    double totalAllowances = 0.0;
    double totalPf = 0.0;
    double totalTds = 0.0;
    double totalAdvances = 0.0;
    double totalOther = 0.0;
    double totalNetPayable = 0.0;
    double totalPostedAmount = 0.0;
};

class SalaryRegisterController : public QObject {
    Q_OBJECT

public:
    explicit SalaryRegisterController(QObject* parent = nullptr);

    // Data operations
    void loadPayroll(int year, int month);
    void refresh();
    
    const QVector<EmployeeSalaryItem>& items() const { return m_items; }
    SalaryRegisterSummary summary() const { return m_summary; }

    int currentYear() const { return m_currentYear; }
    int currentMonth() const { return m_currentMonth; }
    QString currentMonthLabel() const;

    // Calculation helper
    static double calculateEarnedBasic(double baseSalary, int daysInMonth, double presentDays);
    static double calculateNet(double earnedBasic, double allowances, double pf, double tds, double advance, double other);
    static void resolveDesignationAndStation(const QString& rawName, const QString& rawContact, const QString& rawStation, const QString& rawCity, QString& outDesignation, QString& outStation);

    // Update item in memory
    void updateItemCalculations(int index);
    void setItemPresentDays(int index, double days);
    void setItemAllowances(int index, double allowances);
    void setItemPf(int index, double pf);
    void setItemTds(int index, double tds);
    void setItemAdvance(int index, double advance);
    void setItemOther(int index, double other);
    void setItemSelected(int index, bool selected);
    void selectAll(bool select);
    void selectPendingOnly();

    // Master updates
    bool updateMasterSalary(int partyId, double newSalary);

    // Voucher Posting
    struct PostResult {
        bool success = false;
        int vouchersCreated = 0;
        double totalAmountPosted = 0.0;
        QString errorMessage;
        QStringList createdVoucherNumbers;
    };

    PostResult postSelectedSalaries(const QDate& voucherDate, const QString& customNarration = "");

    // Sync from Bahi-Khata MDB files
    int syncMasterSalariesFromBahiKhata(const QString& explicitMdbPath = "");

signals:
    void dataLoaded();
    void summaryChanged();
    void postingFinished(bool success, const QString& message);

private:
    int m_currentYear = 2025;
    int m_currentMonth = 4; // April
    QVector<EmployeeSalaryItem> m_items;
    SalaryRegisterSummary m_summary;

    void recalculateSummary();
    QString getMonthYearKey(int year, int month) const;
};

} // namespace MahadevERP
