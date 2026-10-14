#include "balance_sheet_report.h"
#include "balance_sheet_ca.h"
#include "report_format.h"
#include "report_data.h"
#include "report_html_chrome.h"
#include "../../database_manager.h"
#include "../../engine/accounting_engine.h"
#include "../../engine/fiscal_year_helper.h"
#include "../../engine/profit_loss_calculator.h"
#include "../../engine/balance_sheet_calculator.h"
#include "../../engine/stock_valuation_engine.h"
#include "../../models/account_classifier.h"
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QDebug>
#include <cmath>
#include <algorithm>

namespace PrintReports {

QString renderBalanceSheetHtml(const QString& asOnDate) {
    auto firm = getFirmProfile();
    BalanceSheetData bs = BalanceSheetCalculator::calculate(asOnDate);

    // Same CA stationery as the live widget printer (balance_sheet_ca.h):
    // values come straight from the calculator in both paths.
    QString firmName = bs.firmName.trimmed();
    if (firmName.isEmpty()) firmName = firm.value("firm_name").toString();
    QString dots = FiscalYearHelper::formatDisplayDate(bs.asOnDate);
    dots.replace("-", ".");
    return ErpPrint::balanceSheetCaHtml(firmName, dots,
                                       bs.liabilitiesGroups,
                                       bs.assetsGroups,
                                       bs.totalLiabilities,
                                       bs.totalAssets);
}


} // namespace PrintReports
