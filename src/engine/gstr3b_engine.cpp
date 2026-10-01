#include "gstr3b_engine.h"
#include "gst_tax_engine.h"
#include "../database_manager.h"
#include <QRegularExpression>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <miniz.h>
#include <cmath>

namespace MahadevERP {

static double round2(double val) {
    return std::round(val * 100.0) / 100.0;
}

static QString getMonthName(int month) {
    switch (month) {
        case 1: return "January";
        case 2: return "February";
        case 3: return "March";
        case 4: return "April";
        case 5: return "May";
        case 6: return "June";
        case 7: return "July";
        case 8: return "August";
        case 9: return "September";
        case 10: return "October";
        case 11: return "November";
        case 12: return "December";
        default: return "";
    }
}

static void setCellNumeric(QString& xml, const QString& cellRef, double value) {
    QString valStr = QString::number(value, 'f', 2);
    
    // Pattern 1: self-closing tag <c r="D17" s="67"/> or <c r="D17" s="67" />
    QRegularExpression selfClosing(QString("<c\\s+([^>]*?)r=\"%1\"([^>]*?)\\s*/>").arg(cellRef));
    if (selfClosing.match(xml).hasMatch()) {
        xml.replace(selfClosing, QString("<c \\1r=\"%1\"\\2><v>%2</v></c>").arg(cellRef, valStr));
        return;
    }
    // Pattern 2: open and close tag <c ... r="D17" ...>...</c>
    QRegularExpression openClose(QString("<c\\s+([^>]*?)r=\"%1\"([^>]*?)>.*?</c>").arg(cellRef));
    if (openClose.match(xml).hasMatch()) {
        xml.replace(openClose, QString("<c \\1r=\"%1\"\\2><v>%2</v></c>").arg(cellRef, valStr));
        return;
    }
}

static void setCellString(QString& xml, const QString& cellRef, const QString& text) {
    // Pattern 1: self-closing tag
    QRegularExpression selfClosing(QString("<c\\s+([^>]*?)r=\"%1\"([^>]*?)\\s*/>").arg(cellRef));
    auto match1 = selfClosing.match(xml);
    if (match1.hasMatch()) {
        QString attrs = (match1.captured(1) + " " + match1.captured(2)).trimmed();
        attrs.remove(QRegularExpression("t=\"[^\"]*\""));
        xml.replace(selfClosing, QString("<c r=\"%1\" %2 t=\"inlineStr\"><is><t>%3</t></is></c>").arg(cellRef, attrs.trimmed(), text.toHtmlEscaped()));
        return;
    }
    // Pattern 2: open and close tag
    QRegularExpression openClose(QString("<c\\s+([^>]*?)r=\"%1\"([^>]*?)>.*?</c>").arg(cellRef));
    auto match2 = openClose.match(xml);
    if (match2.hasMatch()) {
        QString attrs = (match2.captured(1) + " " + match2.captured(2)).trimmed();
        attrs.remove(QRegularExpression("t=\"[^\"]*\""));
        xml.replace(openClose, QString("<c r=\"%1\" %2 t=\"inlineStr\"><is><t>%3</t></is></c>").arg(cellRef, attrs.trimmed(), text.toHtmlEscaped()));
        return;
    }
}

Gstr3BReturnSummary Gstr3BEngine::generateFromDatabase(
    const QString& gstin,
    const QString& legalName,
    const QString& stateCode,
    const QDate& fromDate,
    const QDate& toDate
) {
    Gstr3BReturnSummary summary;
    summary.gstin = gstin.trimmed().toUpper();
    summary.legalName = legalName.trimmed();
    summary.tradeName = legalName.trimmed();
    summary.fromDate = fromDate;
    summary.toDate = toDate;
    summary.returnPeriod = QString("%1%2").arg(fromDate.month(), 2, 10, QChar('0')).arg(fromDate.year());
    summary.monthName = getMonthName(fromDate.month());

    int startYear = fromDate.month() >= 4 ? fromDate.year() : fromDate.year() - 1;
    summary.fiscalYear = QString("%1-%2").arg(startYear).arg((startYear + 1) % 100, 2, 10, QChar('0'));

    QString fromStr = fromDate.toString("yyyy-MM-dd");
    QString toStr = toDate.toString("yyyy-MM-dd");

    // 1. Process Outward Supplies (Sales) for Table 3.1
    QString salesSql = QString(
        "SELECT "
        "  s.id, s.total_amount, "
        "  COALESCE(s.taxable_amount, s.total_amount) AS amount, "
        "  COALESCE(s.gst_amount, s.cgst_amount + s.sgst_amount + s.igst_amount, 0.0) AS tax_amount, "
        "  COALESCE(s.cgst_amount, 0.0) AS cgst_amount, "
        "  COALESCE(s.sgst_amount, 0.0) AS sgst_amount, "
        "  COALESCE(s.igst_amount, 0.0) AS igst_amount, "
        "  COALESCE(s.gstin, '') AS party_gstin, "
        "  COALESCE(s.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(s.tax_status, '') AS tax_status, "
        "  COALESCE(s.gst_pct, stk.gst_rate, 0.0) AS tax_rate "
        "FROM sales_invoices s "
        "LEFT JOIN stock_items stk ON (s.item_id = stk.id OR stk.name = s.item_name) "
        "WHERE s.invoice_date >= '%1' AND s.invoice_date <= '%2';"
    ).arg(fromStr, toStr);

    QVariantList salesRows = DatabaseManager::instance().executeQuery(salesSql);
    for (const auto& var : salesRows) {
        QVariantMap r = var.toMap();
        double amt = r.value("amount").toDouble();
        double cgst = r.value("cgst_amount").toDouble();
        double sgst = r.value("sgst_amount").toDouble();
        double igst = r.value("igst_amount").toDouble();
        double taxAmt = r.value("tax_amount").toDouble();
        QString taxStatus = r.value("tax_status").toString().trimmed();

        if (taxAmt > 0.001 || (cgst + sgst + igst > 0.001)) {
            if (taxStatus.contains("Zero", Qt::CaseInsensitive) || taxStatus.contains("Export", Qt::CaseInsensitive)) {
                summary.table31.txValB += amt;
                summary.table31.iAmtB += igst;
            } else {
                summary.table31.txValA += amt;
                summary.table31.cAmtA += cgst;
                summary.table31.sAmtA += sgst;
                summary.table31.iAmtA += igst;
            }
        } else if (taxStatus.contains("Non-GST", Qt::CaseInsensitive)) {
            summary.table31.txValE += amt;
        } else {
            // Nil rated / Exempted sales (e.g. Rice, Paddy, Husk exempt sales)
            summary.table31.txValC += amt;
        }
    }

    // 2. Process Inward Supplies (Purchases) for Table 4 ITC & Table 5 Exempt Inward
    QString purcSql = QString(
        "SELECT "
        "  p.id, p.supplier_id, p.total_amount, "
        "  COALESCE(p.taxable_amount, p.total_amount) AS amount, "
        "  COALESCE(p.gst_amount, p.cgst_amount + p.sgst_amount + p.igst_amount, 0.0) AS tax_amount, "
        "  COALESCE(p.cgst_amount, 0.0) AS cgst_amount, "
        "  COALESCE(p.sgst_amount, 0.0) AS sgst_amount, "
        "  COALESCE(p.igst_amount, 0.0) AS igst_amount, "
        "  COALESCE(p.gstin, '') AS party_gstin, "
        "  COALESCE(p.place_of_supply, '') AS place_of_supply, "
        "  COALESCE(p.tax_status, '') AS tax_status, "
        "  COALESCE(p.gst_pct, stk.gst_rate, 0.0) AS tax_rate, "
        "  COALESCE(pt.state_code, '') AS party_state_code "
        "FROM purchase_invoices p "
        "LEFT JOIN parties pt ON p.supplier_id = pt.id "
        "LEFT JOIN stock_items stk ON (p.item_id = stk.id OR stk.name = p.item_name) "
        "WHERE p.invoice_date >= '%1' AND p.invoice_date <= '%2';"
    ).arg(fromStr, toStr);

    QVariantList purcRows = DatabaseManager::instance().executeQuery(purcSql);
    for (const auto& var : purcRows) {
        QVariantMap r = var.toMap();
        double amt = r.value("amount").toDouble();
        double cgst = r.value("cgst_amount").toDouble();
        double sgst = r.value("sgst_amount").toDouble();
        double igst = r.value("igst_amount").toDouble();
        double taxAmt = r.value("tax_amount").toDouble();
        QString taxStatus = r.value("tax_status").toString().trimmed();

        if (taxAmt > 0.001 || (cgst + sgst + igst > 0.001)) {
            if (taxStatus.contains("RCM", Qt::CaseInsensitive)) {
                summary.table31.txValD += amt;
                summary.table31.cAmtD += cgst;
                summary.table31.sAmtD += sgst;
                summary.table31.iAmtD += igst;

                summary.table4.cAmtA3 += cgst;
                summary.table4.sAmtA3 += sgst;
                summary.table4.iAmtA3 += igst;
            } else {
                summary.table4.cAmtA5 += cgst;
                summary.table4.sAmtA5 += sgst;
                summary.table4.iAmtA5 += igst;
            }
        } else {
            // Exempt / Nil rated / Composition Inward Supplies -> Table 5
            QString sState = GstTaxEngine::extractStateCodeFromGstin(r.value("party_gstin").toString());
            if (sState.isEmpty()) sState = r.value("party_state_code").toString().trimmed();
            if (sState.isEmpty()) sState = r.value("place_of_supply").toString().trimmed();
            if (sState.isEmpty() || sState == "0") sState = stateCode;

            bool isIntra = (sState == stateCode || sState == "06" || sState == "6");

            if (taxStatus.contains("Non-GST", Qt::CaseInsensitive)) {
                if (isIntra) summary.table5.intraNonGst += amt;
                else summary.table5.interNonGst += amt;
            } else {
                if (isIntra) summary.table5.intraExempt += amt;
                else summary.table5.interExempt += amt;
            }
        }
    }

    // 3. Round Table 3.1 values
    summary.table31.txValA = round2(summary.table31.txValA);
    summary.table31.iAmtA = round2(summary.table31.iAmtA);
    summary.table31.cAmtA = round2(summary.table31.cAmtA);
    summary.table31.sAmtA = round2(summary.table31.sAmtA);
    summary.table31.csAmtA = round2(summary.table31.csAmtA);

    summary.table31.txValB = round2(summary.table31.txValB);
    summary.table31.iAmtB = round2(summary.table31.iAmtB);
    summary.table31.csAmtB = round2(summary.table31.csAmtB);

    summary.table31.txValC = round2(summary.table31.txValC);

    summary.table31.txValD = round2(summary.table31.txValD);
    summary.table31.iAmtD = round2(summary.table31.iAmtD);
    summary.table31.cAmtD = round2(summary.table31.cAmtD);
    summary.table31.sAmtD = round2(summary.table31.sAmtD);
    summary.table31.csAmtD = round2(summary.table31.csAmtD);

    summary.table31.txValE = round2(summary.table31.txValE);

    // 4. Round Table 4 ITC values
    summary.table4.iAmtA5 = round2(summary.table4.iAmtA5);
    summary.table4.cAmtA5 = round2(summary.table4.cAmtA5);
    summary.table4.sAmtA5 = round2(summary.table4.sAmtA5);
    summary.table4.csAmtA5 = round2(summary.table4.csAmtA5);

    summary.table4.netIgst = round2(summary.table4.iAmtA1 + summary.table4.iAmtA2 + summary.table4.iAmtA3 + summary.table4.iAmtA4 + summary.table4.iAmtA5 - summary.table4.iAmtB1 - summary.table4.iAmtB2);
    summary.table4.netCgst = round2(summary.table4.cAmtA1 + summary.table4.cAmtA2 + summary.table4.cAmtA3 + summary.table4.cAmtA4 + summary.table4.cAmtA5 - summary.table4.cAmtB1 - summary.table4.cAmtB2);
    summary.table4.netSgst = round2(summary.table4.sAmtA1 + summary.table4.sAmtA2 + summary.table4.sAmtA3 + summary.table4.sAmtA4 + summary.table4.sAmtA5 - summary.table4.sAmtB1 - summary.table4.sAmtB2);
    summary.table4.netCess = round2(summary.table4.csAmtA1 + summary.table4.csAmtA2 + summary.table4.csAmtA3 + summary.table4.csAmtA4 + summary.table4.csAmtA5 - summary.table4.csAmtB1 - summary.table4.csAmtB2);

    // 5. Round Table 5 values
    summary.table5.interExempt = round2(summary.table5.interExempt);
    summary.table5.intraExempt = round2(summary.table5.intraExempt);
    summary.table5.interNonGst = round2(summary.table5.interNonGst);
    summary.table5.intraNonGst = round2(summary.table5.intraNonGst);

    // 6. Calculate Table 6.1 Tax Payment & Net Cash Payable
    summary.table61.taxPayableIgst = round2(summary.table31.iAmtA + summary.table31.iAmtB + summary.table31.iAmtD);
    summary.table61.taxPayableCgst = round2(summary.table31.cAmtA + summary.table31.cAmtD);
    summary.table61.taxPayableSgst = round2(summary.table31.sAmtA + summary.table31.sAmtD);
    summary.table61.taxPayableCess = round2(summary.table31.csAmtA + summary.table31.csAmtB + summary.table31.csAmtD);

    summary.table61.itcPaidIgst = std::min(summary.table61.taxPayableIgst, summary.table4.netIgst);
    summary.table61.itcPaidCgst = std::min(summary.table61.taxPayableCgst, summary.table4.netCgst);
    summary.table61.itcPaidSgst = std::min(summary.table61.taxPayableSgst, summary.table4.netSgst);
    summary.table61.itcPaidCess = std::min(summary.table61.taxPayableCess, summary.table4.netCess);

    summary.table61.cashPaidIgst = std::max(0.0, round2(summary.table61.taxPayableIgst - summary.table61.itcPaidIgst));
    summary.table61.cashPaidCgst = std::max(0.0, round2(summary.table61.taxPayableCgst - summary.table61.itcPaidCgst));
    summary.table61.cashPaidSgst = std::max(0.0, round2(summary.table61.taxPayableSgst - summary.table61.itcPaidSgst));
    summary.table61.cashPaidCess = std::max(0.0, round2(summary.table61.taxPayableCess - summary.table61.itcPaidCess));

    summary.netPayableIgst = summary.table61.cashPaidIgst;
    summary.netPayableCgst = summary.table61.cashPaidCgst;
    summary.netPayableSgst = summary.table61.cashPaidSgst;
    summary.netPayableTotal = round2(summary.netPayableIgst + summary.netPayableCgst + summary.netPayableSgst);

    return summary;
}

QString Gstr3BEngine::findTemplatePath(const QString& preferredPath) {
    if (!preferredPath.isEmpty() && QFile::exists(preferredPath)) {
        return preferredPath;
    }

    QStringList candidates = {
        QDir::currentPath() + "/gst-data/GSTR-3B.xls",
        QDir::currentPath() + "/data/GSTR-3B.xls",
        QDir::currentPath() + "/GSTR-3B.xls",
        QCoreApplication::applicationDirPath() + "/gst-data/GSTR-3B.xls",
        QCoreApplication::applicationDirPath() + "/../Resources/templates/GSTR-3B.xls",
        QDir::homePath() + "/MahadevAc/gst-data/GSTR-3B.xls"
    };

    for (const auto& path : candidates) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    return "";
}

bool Gstr3BEngine::exportToExcelTemplate(
    const Gstr3BReturnSummary& summary,
    const QString& outputPath,
    const QString& templatePath
) {
    QString tPath = findTemplatePath(templatePath);
    if (tPath.isEmpty()) {
        return false;
    }

    QFile tFile(tPath);
    if (!tFile.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray tBytes = tFile.readAll();
    tFile.close();

    mz_zip_archive inZip;
    memset(&inZip, 0, sizeof(inZip));
    if (!mz_zip_reader_init_mem(&inZip, tBytes.constData(), tBytes.size(), 0)) {
        return false;
    }

    mz_zip_archive outZip;
    memset(&outZip, 0, sizeof(outZip));
    if (!mz_zip_writer_init_heap(&outZip, 0, 0)) {
        mz_zip_reader_end(&inZip);
        return false;
    }

    mz_uint numFiles = mz_zip_reader_get_num_files(&inZip);
    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&inZip, i, &file_stat)) continue;

        size_t uncompSize = 0;
        void* pData = mz_zip_reader_extract_to_heap(&inZip, i, &uncompSize, 0);
        if (!pData) continue;

        QByteArray data((const char*)pData, (int)uncompSize);
        mz_free(pData);

        QString fn = QString::fromUtf8(file_stat.m_filename);
        if (fn == "xl/worksheets/sheet1.xml") {
            QString xml = QString::fromUtf8(data);

            // 1. Header Information
            setCellString(xml, "Q5", summary.fiscalYear);
            setCellString(xml, "Q6", summary.monthName);
            setCellString(xml, "D8", summary.gstin);
            setCellString(xml, "D9", summary.legalName);

            // 2. Table 3.1 Outward Supplies
            setCellNumeric(xml, "D15", summary.table31.txValA);
            setCellNumeric(xml, "G15", summary.table31.iAmtA);
            setCellNumeric(xml, "J15", summary.table31.cAmtA);
            setCellNumeric(xml, "M15", summary.table31.sAmtA);
            setCellNumeric(xml, "P15", summary.table31.csAmtA);

            setCellNumeric(xml, "D16", summary.table31.txValB);
            setCellNumeric(xml, "G16", summary.table31.iAmtB);
            setCellNumeric(xml, "P16", summary.table31.csAmtB);

            setCellNumeric(xml, "D17", summary.table31.txValC);

            setCellNumeric(xml, "D18", summary.table31.txValD);
            setCellNumeric(xml, "G18", summary.table31.iAmtD);
            setCellNumeric(xml, "J18", summary.table31.cAmtD);
            setCellNumeric(xml, "M18", summary.table31.sAmtD);
            setCellNumeric(xml, "P18", summary.table31.csAmtD);

            setCellNumeric(xml, "D19", summary.table31.txValE);

            // 3. Table 4 Eligible ITC
            setCellNumeric(xml, "D35", summary.table4.iAmtA1);
            setCellNumeric(xml, "H35", summary.table4.cAmtA1);
            setCellNumeric(xml, "L35", summary.table4.sAmtA1);
            setCellNumeric(xml, "P35", summary.table4.csAmtA1);

            setCellNumeric(xml, "D36", summary.table4.iAmtA2);
            setCellNumeric(xml, "H36", summary.table4.cAmtA2);
            setCellNumeric(xml, "L36", summary.table4.sAmtA2);
            setCellNumeric(xml, "P36", summary.table4.csAmtA2);

            setCellNumeric(xml, "D37", summary.table4.iAmtA3);
            setCellNumeric(xml, "H37", summary.table4.cAmtA3);
            setCellNumeric(xml, "L37", summary.table4.sAmtA3);
            setCellNumeric(xml, "P37", summary.table4.csAmtA3);

            setCellNumeric(xml, "D39", summary.table4.iAmtA4);
            setCellNumeric(xml, "H39", summary.table4.cAmtA4);
            setCellNumeric(xml, "L39", summary.table4.sAmtA4);
            setCellNumeric(xml, "P39", summary.table4.csAmtA4);

            setCellNumeric(xml, "D40", summary.table4.iAmtA5);
            setCellNumeric(xml, "H40", summary.table4.cAmtA5);
            setCellNumeric(xml, "L40", summary.table4.sAmtA5);
            setCellNumeric(xml, "P40", summary.table4.csAmtA5);

            setCellNumeric(xml, "D42", summary.table4.iAmtB1);
            setCellNumeric(xml, "H42", summary.table4.cAmtB1);
            setCellNumeric(xml, "L42", summary.table4.sAmtB1);
            setCellNumeric(xml, "P42", summary.table4.csAmtB1);

            setCellNumeric(xml, "D43", summary.table4.iAmtB2);
            setCellNumeric(xml, "H43", summary.table4.cAmtB2);
            setCellNumeric(xml, "L43", summary.table4.sAmtB2);
            setCellNumeric(xml, "P43", summary.table4.csAmtB2);

            setCellNumeric(xml, "D46", summary.table4.iAmtD1);
            setCellNumeric(xml, "H46", summary.table4.cAmtD1);
            setCellNumeric(xml, "L46", summary.table4.sAmtD1);
            setCellNumeric(xml, "P46", summary.table4.csAmtD1);

            setCellNumeric(xml, "D47", summary.table4.iAmtD2);
            setCellNumeric(xml, "H47", summary.table4.cAmtD2);
            setCellNumeric(xml, "L47", summary.table4.sAmtD2);
            setCellNumeric(xml, "P47", summary.table4.csAmtD2);

            // 4. Table 5 Exempt Inward Supplies
            setCellNumeric(xml, "E53", summary.table5.interExempt);
            setCellNumeric(xml, "L53", summary.table5.intraExempt);
            setCellNumeric(xml, "E54", summary.table5.interNonGst);
            setCellNumeric(xml, "L54", summary.table5.intraNonGst);

            // 5. Table 6.1 Late Fee & Interest
            if (summary.table51.lateFeeCgst > 0.0) setCellNumeric(xml, "R62", summary.table51.lateFeeCgst);
            if (summary.table51.lateFeeSgst > 0.0) setCellNumeric(xml, "R63", summary.table51.lateFeeSgst);
            if (summary.table51.interestIgst > 0.0) setCellNumeric(xml, "Q61", summary.table51.interestIgst);
            if (summary.table51.interestCgst > 0.0) setCellNumeric(xml, "Q62", summary.table51.interestCgst);
            if (summary.table51.interestSgst > 0.0) setCellNumeric(xml, "Q63", summary.table51.interestSgst);
            if (summary.table51.interestCess > 0.0) setCellNumeric(xml, "Q64", summary.table51.interestCess);

            data = xml.toUtf8();
        }

        mz_zip_writer_add_mem(&outZip, file_stat.m_filename, data.constData(), data.size(), MZ_DEFAULT_COMPRESSION);
    }

    void* pOutBuf = nullptr;
    size_t outSize = 0;
    mz_zip_writer_finalize_heap_archive(&outZip, &pOutBuf, &outSize);
    mz_zip_reader_end(&inZip);

    QFileInfo outInfo(outputPath);
    QDir().mkpath(outInfo.absolutePath());

    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        mz_free(pOutBuf);
        mz_zip_writer_end(&outZip);
        return false;
    }

    outFile.write((const char*)pOutBuf, (qint64)outSize);
    outFile.close();

    mz_free(pOutBuf);
    mz_zip_writer_end(&outZip);
    return true;
}

} // namespace MahadevERP
