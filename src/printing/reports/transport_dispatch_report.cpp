#include "transport_dispatch_report.h"
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

QString renderTransportDispatchHtml(int dispatchId) {
    auto firm = getFirmProfile();

    QVariantList rows = DatabaseManager::instance().executeQuery(
        "SELECT * FROM transport_dispatches WHERE id = ? LIMIT 1;", {dispatchId}
    );
    if (rows.isEmpty()) {
        return "<h2>Dispatch record not found</h2>";
    }

    QVariantMap r = rows.first().toMap();
    QString dDate = FiscalYearHelper::formatDisplayDate(r.value("dispatch_date").toString());
    QString vehNo = r.value("vehicle_no").toString();
    QString driver = r.value("driver_name").toString();
    QString party = r.value("party_name").toString();
    QString item = r.value("item_name").toString();
    long long bags = r.value("bag_count").toLongLong();
    double netWt = r.value("net_weight_qtl").toDouble();
    double grossWt = r.value("gross_weight_qtl").toDouble();
    double tareWt = r.value("tare_weight_qtl").toDouble();

    QString body = ErpPrint::letterheadHtml(firm)
        + ErpPrint::titleBlockHtml("GATE PASS / TRANSPORT DISPATCH MEMO",
            QString("Pass No: GP-%1  |  Date: %2")
                .arg(QString::number(dispatchId), dDate))
        + QString(
        "<table width='100%' border='0' cellspacing='1' cellpadding='6' bgcolor='#94A3B8'>"
        "  <tr>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Vehicle No:</b> <font size='3'><b>%1</b></font><br><b>Driver:</b> %2</td>"
        "    <td width='50%' bgcolor='#FFFFFF'><b>Consignee / Destination:</b><br><font size='3'><b>%3</b></font></td>"
        "  </tr>"
        "  <tr>"
        "    <td bgcolor='#FFFFFF'><b>Commodity:</b> <b>%4</b><br><b>Bags Count:</b> %5 Bags</td>"
        "    <td bgcolor='#FFFFFF'><b>Gross Weight:</b> %6 Qtl<br><b>Tare Weight:</b> %7 Qtl<br><b>NET WEIGHT:</b> <b>%8 Qtl</b></td>"
        "  </tr>"
        "</table>")
        .arg(vehNo.toHtmlEscaped())
        .arg((driver.isEmpty() ? "N/A" : driver).toHtmlEscaped())
        .arg(party.toHtmlEscaped())
        .arg(item.toHtmlEscaped())
        .arg(bags)
        .arg(QString::number(grossWt, 'f', 2))
        .arg(QString::number(tareWt, 'f', 2))
        .arg(QString::number(netWt, 'f', 2))
        + ErpPrint::voucherSignHtml(QStringList() << "Driver Signature" << "Weighbridge Incharge" << "Security Gate Pass Out");

    return ErpPrint::wrapPrintDocument(body);
}


} // namespace PrintReports
