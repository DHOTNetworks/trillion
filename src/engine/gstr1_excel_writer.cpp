#include "gstr1_excel_writer.h"
#include <miniz.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLocale>
#include <QMap>
#include <QRegularExpression>

namespace MahadevERP {

namespace {

// ---------- tiny xml helpers ----------

QString esc(const QString& s) {
    QString o = s;
    o.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return o;
}

QString colName(int idx0) { // 0 -> A
    QString c;
    int n = idx0;
    do {
        c.prepend(QChar('A' + (n % 26)));
        n = n / 26 - 1;
    } while (n >= 0);
    return c;
}

QString cellRef(int row1, int col0) {
    return colName(col0) + QString::number(row1);
}

QString cellText(const QString& ref, const QString& text, bool bold = false) {
    return QString("<c r=\"%1\" t=\"inlineStr\"%2><is><t xml:space=\"preserve\">%3</t></is></c>")
        .arg(ref, bold ? " s=\"1\"" : "", esc(text));
}

QString cellNum(const QString& ref, const QString& num) {
    return QString("<c r=\"%1\"><v>%2</v></c>").arg(ref, num);
}

QString amt(double v) {
    return QString::number(v, 'f', 2);
}

QString rateNum(double v) {
    QString s = QString::number(v, 'g', 12);
    if (s.contains('e') || s.contains('E')) s = QString::number(v, 'f', 10);
    return s;
}

QString xlsDate(const QDate& d) {
    if (!d.isValid()) return QString();
    return QLocale::c().toString(d, "dd-MMM-yyyy"); // dd-mmm-yyyy, official tool format
}

// ---------- master lists (verbatim from the official template; FYEAR extended) ----------

QStringList masterCol(const QString& key) {
    static const QMap<QString, QStringList> k = {
        {"UQC", {"BAG-BAGS","BAL-BALE","BDL-BUNDLES","BKL-BUCKLES","BOU-BILLION OF UNITS","BOX-BOX","BTL-BOTTLES","BUN-BUNCHES","CAN-CANS","CBM-CUBIC METERS","CCM-CUBIC CENTIMETERS","CMS-CENTIMETERS","CTN-CARTONS","DOZ-DOZENS","DRM-DRUMS","GGK-GREAT GROSS","GMS-GRAMMES","GRS-GROSS","GYD-GROSS YARDS","KGS-KILOGRAMS","KLR-KILOLITRE","KME-KILOMETRE","MLT-MILILITRE","MTR-METERS","MTS-METRIC TON","NOS-NUMBERS","PAC-PACKS","PCS-PIECES","PRS-PAIRS","QTL-QUINTAL","ROL-ROLLS","SET-SETS","SQF-SQUARE FEET","SQM-SQUARE METERS","SQY-SQUARE YARDS","TBS-TABLETS","TGM-TEN GROSS","THD-THOUSANDS","TON-TONNES","TUB-TUBES","UGS-US GALLONS","UNT-UNITS","YDS-YARDS","OTH-OTHERS"}},
        {"EXP", {"WOPAY","WPAY"}},
        {"RCHARGE", {"N","Y"}},
        {"NOTE", {"C","D","R"}},
        {"TYPE", {"OE","E"}},
        {"RATE", {"0","0.1","0.25","1","1.5","3","5","6","7.5","12","18","28","40"}},
        {"POS", {"01-Jammu & Kashmir","02-Himachal Pradesh","03-Punjab","04-Chandigarh","05-Uttarakhand","06-Haryana","07-Delhi","08-Rajasthan","09-Uttar Pradesh","10-Bihar","11-Sikkim","12-Arunachal Pradesh","13-Nagaland","14-Manipur","15-Mizoram","16-Tripura","17-Meghalaya","18-Assam","19-West Bengal","20-Jharkhand","21-Odisha","22-Chhattisgarh","23-Madhya Pradesh","24-Gujarat","25-Daman & Diu","26-Dadra & Nagar Haveli","27-Maharashtra","29-Karnataka","30-Goa","31-Lakshdweep","32-Kerala","33-Tamil Nadu","34-Puducherry","35-Andaman & Nicobar Islands","36-Telangana","37-Andhra Pradesh","38-Ladakh","97-Other Territory"}},
        {"INVTYPE", {"Regular","SEZ supplies with payment","SEZ supplies without payment","Deemed Exp","Intra-State supplies attracting IGST"}},
        {"NOTERSN", {"01-Sales Return","02-Post Sale Discount","03-Deficiency in services","04-Correction in Invoice","05-Change in POS","06-Finalization of Provisional assessment","07-Others"}},
        {"DOCUMENT", {"Invoices for outward supply","Invoices for inward supply from unregistered person","Revised Invoice","Debit Note","Credit Note","Receipt Voucher","Payment Voucher","Refund Voucher","Delivery Challan for job work","Delivery Challan for supply on approval","Delivery Challan in case of liquid gas","Delivery Challan in case other than by way of supply (excluding at S no. 9 to 11)"}},
        {"URTYPE", {"B2CL","EXPWP","EXPWOP"}},
        {"STYPE", {"Inter State","Intra State"}},
        {"MONTH", {"JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"}},
        {"FYEAR", {"2017-18","2018-19","2019-20","2020-21","2021-22","2022-23","2023-24","2024-25","2025-26","2026-27","2027-28","2028-29"}},
        {"DIFF", {"65.00"}},
        {"ECO_SUP", {"Liable to collect tax u/s 52(TCS)","Liable to pay tax u/s 9(5)"}},
    };
    return k.value(key);
}

QString uqcDisplay(const QString& code) {
    for (const QString& e : masterCol("UQC")) {
        if (e.left(e.indexOf('-')) == code.trimmed().toUpper()) return e;
    }
    return "OTH-OTHERS";
}

QString invTypeDisplay(const QString& t) {
    QString u = t.trimmed().toUpper();
    if (u == "SEWP") return "SEZ supplies with payment";
    if (u == "SEWOP") return "SEZ supplies without payment";
    if (u == "DE") return "Deemed Exp";
    return "Regular";
}

// ---------- sheet scaffolding ----------

struct SheetSpec {
    QString name;
    QString title;              // row 1 col A
    QMap<int, QString> row1Extra; // col -> label (row 1)
    QMap<int, QString> row2;      // col -> label (row 2)
    QStringList headers;          // row 4
    bool freeze = false;
};

QVector<SheetSpec> sheetSpecs() {
    QVector<SheetSpec> v;
    SheetSpec s;
    s.name = "b2b"; s.title = "Summary For B2B(4)"; s.freeze = true;
    s.row2 = {{0,"No. of Recipients"},{2,"No. of Invoices"},{4,"Total Invoice Value"},{11,"Total Taxable Value"},{12,"Total Cess"}};
    s.headers = {"GSTIN/UIN of Recipient","Receiver Name","Invoice Number","Invoice date","Invoice Value","Place Of Supply","Reverse Charge","Applicable % of Tax Rate","Invoice Type","E-Commerce GSTIN","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "b2cl"; s.title = "Summary For B2CL(5)"; s.freeze = true;
    s.row2 = {{0,"No. of Invoices"},{2,"Total Inv Value"},{6,"Total Taxable Value"},{7,"Total Cess"}};
    s.headers = {"Invoice Number","Invoice date","Invoice Value","Place Of Supply","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount","E-Commerce GSTIN"};
    v.append(s);
    s = SheetSpec(); s.name = "b2cs"; s.title = "Summary For B2CS(7)"; s.freeze = true;
    s.row2 = {{4,"Total Taxable  Value"},{5,"Total Cess"}};
    s.headers = {"Type","Place Of Supply","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount","E-Commerce GSTIN"};
    v.append(s);
    s = SheetSpec(); s.name = "cdnr"; s.title = "Summary For CDNR(9B)"; s.freeze = true;
    s.row2 = {{0,"No. of Recipients"},{2,"No. of Notes/Vouchers"},{8,"Total Note/Refund Voucher Value"},{11,"Total Taxable Value"},{12,"Total Cess"}};
    s.headers = {"GSTIN/UIN of Recipient","Receiver Name","Note/Refund Voucher Number","Note/Refund Voucher date","Note Type","Place Of Supply","Reverse Charge","Note Supply Type","Note Value","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "cdnur"; s.title = "Summary For CDNUR(9B)"; s.freeze = true;
    s.row2 = {{1,"No. of Notes/Vouchers"},{5,"Total Note Value"},{8,"Total Taxable Value"},{9,"Total Cess"}};
    s.headers = {"UR Type","Note/Refund Voucher Number","Note/Refund Voucher date","Note Type","Place Of Supply","Note Value","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "exp"; s.title = "Summary For EXP(6)"; s.freeze = true;
    s.row2 = {{1,"No. of Invoices"},{3,"Total Invoice Value"},{4,"No. of Shipping Bill"},{9,"Total Taxable Value"}};
    s.headers = {"Export Type","Invoice Number","Invoice date","Invoice Value","Port Code","Shipping Bill Number","Shipping Bill Date","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "expa"; s.title = "Summary For EXPA"; s.freeze = true;
    s.row1Extra = {{1,"Original details"},{3,"Revised details"}};
    s.row2 = {{1,"No. of Invoices"},{5,"Total Invoice Value"},{7,"No. of Shipping Bill"},{11,"Total Taxable Value"}};
    s.headers = {"Export Type","Original Invoice Number","Original Invoice date","Revised Invoice Number","Revised Invoice date","Invoice Value","Port Code","Shipping Bill Number","Shipping Bill Date","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "at"; s.title = "Summary For Advance Received (11B)"; s.freeze = true;
    s.row2 = {{3,"Total Advance Received"},{4,"Total Cess"}};
    s.headers = {"Place Of Supply","Applicable % of Tax Rate","Rate","Gross Advance Received","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "atadj"; s.title = "Summary For Advance Adjusted (11B)"; s.freeze = true;
    s.row2 = {{3,"Total Advance Adjusted"},{4,"Total Cess"}};
    s.headers = {"Place Of Supply","Applicable % of Tax Rate","Rate","Gross Advance Adjusted","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "exemp"; s.title = "Summary For Nil rated, exempted and non GST outward supplies (8)"; s.freeze = true;
    s.row2 = {{1,"Total Nil Rated Supplies"},{2,"Total Exempted Supplies"},{3,"Total Non-GST Supplies"}};
    s.headers = {"Description","Nil Rated Supplies","Exempted(other than nil rated/non GST supply)","Non-GST Supplies"};
    v.append(s);
    s = SheetSpec(); s.name = "hsn(b2b)"; s.title = "Summary For HSN(12)"; s.freeze = true;
    s.row2 = {{0,"No. of HSN"},{4,"Total Value"},{6,"Total Taxable Value"},{7,"Total Integrated Tax"},{8,"Total Central Tax"},{9,"Total State/UT Tax"},{10,"Total Cess"}};
    s.headers = {"HSN","Description","UQC","Total Quantity","Total Value","Rate","Taxable Value","Integrated Tax Amount","Central Tax Amount","State/UT Tax Amount","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "hsn(b2c)"; s.title = "Summary For HSN(12)"; s.freeze = true;
    s.row2 = {{0,"No. of HSN"},{4,"Total Value"},{6,"Total Taxable Value"},{7,"Total Integrated Tax"},{8,"Total Central Tax"},{9,"Total State/UT Tax"},{10,"Total Cess"}};
    s.headers = {"HSN","Description","UQC","Total Quantity","Total Value","Rate","Taxable Value","Integrated Tax Amount","Central Tax Amount","State/UT Tax Amount","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "docs"; s.title = "Summary of documents issued during the tax period (13)"; s.freeze = true;
    s.row2 = {{3,"Total Number"},{4,"Total Cancelled"}};
    s.headers = {"Nature of Document","Sr. No. From","Sr. No. To","Total Number","Cancelled"};
    v.append(s);
    s = SheetSpec(); s.name = "b2ba"; s.title = "Summary For B2BA";
    s.row1Extra = {{1,"Original details"},{4,"Revised Details"}};
    s.row2 = {{0,"No. of Recipients"},{2,"No. of Invoices"},{6,"Total Invoice Value"},{13,"Total Taxable Value"},{14,"Total Cess"}};
    s.headers = {"GSTIN/UIN of Recipient","Receiver Name","Original Invoice Number","Original Invoice date","Revised Invoice Number","Revised Invoice date","Invoice Value","Place Of Supply","Reverse Charge","Applicable % of Tax Rate","Invoice Type","E-Commerce GSTIN","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "b2csa"; s.title = "Summary For B2CSA";
    s.row1Extra = {{1,"Original details"},{2,"Revised details"}};
    s.row2 = {{6,"Total Taxable  Value"},{7,"Total Cess"}};
    s.headers = {"Financial Year","Original Month","Place Of Supply","Type","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount","E-Commerce GSTIN"};
    v.append(s);
    s = SheetSpec(); s.name = "cdnra"; s.title = "Summary For CDNRA";
    s.row1Extra = {{1,"Original details"},{4,"Revised details"}};
    s.row2 = {{0,"No. of Recipients"},{2,"No. of Notes/Vouchers"},{10,"Total Note/Refund Voucher Value"},{13,"Total Taxable Value"},{14,"Total Cess"}};
    s.headers = {"GSTIN/UIN of Recipient","Receiver Name","Original Note Number","Original Note date","Revised Note Number","Revised Note date","Note Type","Place Of Supply","Reverse Charge","Note Supply Type","Note Value","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "cdnura"; s.title = "Summary For CDNUR(9B)"; s.freeze = true;
    s.row1Extra = {{1,"Original Details"},{3,"Revised Details"}};
    s.row2 = {{1,"No. of Notes/Vouchers"},{7,"Total Note Value"},{10,"Total Taxable Value"},{11,"Total Cess"}};
    s.headers = {"UR Type","Original Note Number","Original Note Date","Revised Note Number","Revised Note Date","Note Type","Place Of Supply","Note Value","Applicable % of Tax Rate","Rate","Taxable Value","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "ata"; s.title = "Summary For Amended Tax Liability(Advance Received)";
    s.row1Extra = {{1,"Original details"},{3,"Revised details"}};
    s.row2 = {{5,"Total Advance Received"},{6,"Total Cess"}};
    s.headers = {"Financial Year","Original Month","Original Place Of Supply","Applicable % of Tax Rate","Rate","Gross Advance Received","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "atadja"; s.title = "Summary For Amendement Of Adjustment Advances";
    s.row1Extra = {{1,"Original details"},{3,"Revised details"}};
    s.row2 = {{5,"Total Advance Received"},{6,"Total Cess"}};
    s.headers = {"Financial Year","Original Month","Original Place Of Supply","Applicable % of Tax Rate","Rate","Gross Advance Adjusted","Cess Amount"};
    v.append(s);
    s = SheetSpec(); s.name = "eco"; s.title = "Summary For Supplies through ECO-14";
    s.row2 = {{1,"No. of E-Commerce Operator"},{3,"Total Net Value of Supplies"},{4,"Total Integrated Tax"},{5,"Total Central Tax"},{6,"Total State/UT Tax"},{7,"Total Cess"}};
    s.headers = {"Nature of Supply","GSTIN of E-Commerce Operator","E-Commerce Operator Name","Net value of supplies","Integrated tax","Central tax","State/UT tax","Cess"};
    v.append(s);
    return v;
}

// Validations: column (0-based) -> defined-name list. Mirrors the template.
QMap<QString, QMap<int, QString>> listValidations() {
    QMap<QString, QMap<int, QString>> m;
    m["b2b"] = {{5,"POS"},{6,"RCHARGE"},{7,"DIFF"},{8,"INVTYPE"},{10,"RATE"}};
    m["b2cl"] = {{3,"POS"},{4,"DIFF"},{5,"RATE"}};
    m["b2cs"] = {{0,"TYPE"},{1,"POS"},{2,"DIFF"},{3,"RATE"}};
    m["cdnr"] = {{4,"CDRNOTE"},{5,"POS"},{6,"RCHARGE"},{7,"INVTYPE"},{9,"DIFF"},{10,"RATE"}};
    m["cdnur"] = {{0,"URTYPE"},{3,"CDRNOTE"},{4,"POS"},{6,"DIFF"},{7,"RATE"}};
    m["exp"] = {{0,"EXP"},{7,"RATE"}};
    m["expa"] = {{0,"EXP"},{9,"RATE"}};
    m["at"] = {{0,"POS"},{1,"DIFF"},{2,"RATE"}};
    m["atadj"] = {{0,"POS"},{1,"DIFF"},{2,"RATE"}};
    m["hsn(b2b)"] = {{2,"NUQC"},{5,"RATE"}};
    m["hsn(b2c)"] = {{2,"NUQC"},{5,"RATE"}};
    m["docs"] = {{0,"DOCUMENT"}};
    m["b2ba"] = {{7,"POS"},{9,"DIFF"},{10,"INVTYPE"},{12,"RATE"}};
    m["b2csa"] = {{0,"FYEAR"},{1,"MONTH"},{2,"POS"},{3,"TYPE"},{4,"DIFF"},{5,"RATE"}};
    m["cdnra"] = {{6,"CDRNOTE"},{7,"POS"},{8,"RCHARGE"},{9,"INVTYPE"},{11,"DIFF"},{12,"RATE"}};
    m["cdnura"] = {{0,"URTYPE"},{5,"CDRNOTE"},{6,"POS"},{8,"DIFF"},{9,"RATE"}};
    m["ata"] = {{0,"FYEAR"},{1,"MONTH"},{2,"POS"},{3,"DIFF"},{4,"RATE"}};
    m["atadja"] = {{0,"FYEAR"},{1,"MONTH"},{2,"POS"},{3,"DIFF"},{4,"RATE"}};
    m["eco"] = {{0,"ECO_SUP"}};
    return m;
}

// Amount columns (0-based) guarded by decimal >= 0.
QMap<QString, QList<int>> amountColumns() {
    QMap<QString, QList<int>> m;
    m["b2b"] = {4, 11, 12};
    m["b2cl"] = {2, 6, 7};
    m["b2cs"] = {4, 5};
    m["cdnr"] = {8, 11, 12};
    m["cdnur"] = {5, 8, 9};
    m["hsn(b2b)"] = {3, 4, 6, 7, 8, 9, 10};
    m["hsn(b2c)"] = {3, 4, 6, 7, 8, 9, 10};
    m["docs"] = {3, 4};
    m["exemp"] = {1, 2, 3};
    return m;
}

QString buildSheet(const SheetSpec& spec, const QVector<QStringList>& textRows,
                   const QVector<QVector<double>>& numRows, int numCols) {
    // textRows/numRows: parallel per data row; empty string / NaN => skip cell.
    QString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">";
    int lastRow = 4 + qMax(textRows.size(), 0);
    if (lastRow < 4) lastRow = 4;
    xml += QString("<dimension ref=\"A1:%1%2\"/>").arg(colName(numCols - 1)).arg(lastRow);
    if (spec.freeze) {
        xml += "<sheetViews><sheetView workbookViewId=\"0\">"
               "<pane ySplit=\"4\" topLeftCell=\"A5\" activePane=\"bottomLeft\" state=\"frozen\"/>"
               "</sheetView></sheetViews>";
    } else {
        xml += "<sheetViews><sheetView workbookViewId=\"0\"/></sheetViews>";
    }
    xml += "<sheetFormat defaultRowHeight=\"15\"/><sheetData>";
    xml += QString("<row r=\"1\">%1").arg(cellText("A1", spec.title, true));
    for (auto it = spec.row1Extra.constBegin(); it != spec.row1Extra.constEnd(); ++it)
        xml += cellText(cellRef(1, it.key()), it.value(), true);
    xml += "</row><row r=\"2\">";
    for (auto it = spec.row2.constBegin(); it != spec.row2.constEnd(); ++it)
        xml += cellText(cellRef(2, it.key()), it.value(), true);
    xml += "</row><row r=\"4\">";
    for (int c = 0; c < spec.headers.size(); c++)
        xml += cellText(cellRef(4, c), spec.headers[c], true);
    xml += "</row>";
    for (int r = 0; r < textRows.size(); r++) {
        xml += QString("<row r=\"%1\">").arg(5 + r);
        const QStringList& tr = textRows[r];
        const QVector<double>& nr = numRows[r];
        for (int c = 0; c < numCols; c++) {
            QString t = (c < tr.size()) ? tr[c] : QString();
            bool hasNum = (c < nr.size()) && !std::isnan(nr[c]);
            if (hasNum) xml += cellNum(cellRef(5 + r, c), amt(nr[c]));
            else if (!t.isEmpty()) xml += cellText(cellRef(5 + r, c), t);
        }
        xml += "</row>";
    }
    xml += "</sheetData>";
    QString dvs;
    int dvCount = 0;
    QMap<int, QString> lists = listValidations().value(spec.name);
    for (auto it = lists.constBegin(); it != lists.constEnd(); ++it) {
        QString col = colName(it.key());
        dvs += QString("<dataValidation type=\"list\" allowBlank=\"1\" showDropDown=\"1\" sqref=\"%1%2:%1%3\">"
                       "<formula1>%4</formula1></dataValidation>")
                   .arg(col).arg(5).arg(1048576).arg(it.value());
        dvCount++;
    }
    for (int c : amountColumns().value(spec.name)) {
        QString col = colName(c);
        dvs += QString("<dataValidation type=\"decimal\" operator=\"greaterThanOrEqual\" allowBlank=\"1\" "
                       "showDropDown=\"1\" showErrorMessage=\"1\" sqref=\"%1%2:%1%3\">"
                       "<formula1>0</formula1></dataValidation>")
                   .arg(col).arg(5).arg(1048576);
        dvCount++;
    }
    if (spec.name == "b2ba") { // inline N/Y like the template (not a named range)
        dvs += "<dataValidation type=\"list\" allowBlank=\"1\" showDropDown=\"1\" sqref=\"I5:I1048576\">"
               "<formula1>\"N,Y\"</formula1></dataValidation>";
        dvCount++;
    }
    if (dvCount > 0) xml += QString("<dataValidations count=\"%1\">%2</dataValidations>").arg(dvCount).arg(dvs);
    xml += "</worksheet>";
    return xml;
}

const double kNaN = std::numeric_limits<double>::quiet_NaN();

} // namespace

Gstr1ExcelResult Gstr1ExcelWriter::write(const Gstr1ReturnPayload& p, const QString& outPath) {
    Gstr1ExcelResult res;
    QVector<SheetSpec> specs = sheetSpecs();

    // ---- data rows per sheet ----
    QMap<QString, QVector<QStringList>> texts;
    QMap<QString, QVector<QVector<double>>> nums;
    auto newRow = [&](const QString& sheet, int cols) {
        texts[sheet].append(QStringList());
        for (int i = 0; i < cols; i++) texts[sheet].last().append(QString());
        nums[sheet].append(QVector<double>());
        for (int i = 0; i < cols; i++) nums[sheet].last().append(kNaN);
        return texts[sheet].size() - 1;
    };
    auto setT = [&](const QString& sheet, int r, int c, const QString& v) { texts[sheet][r][c] = v; };
    auto setN = [&](const QString& sheet, int r, int c, double v) { nums[sheet][r][c] = v; };

    for (const auto& inv : p.b2b) {
        for (int li = 0; li < inv.items.size(); li++) {
            const auto& it = inv.items[li];
            int r = newRow("b2b", 13);
            if (li == 0) {
                setT("b2b", r, 0, inv.ctin);
                setT("b2b", r, 1, inv.receiverName);
                setT("b2b", r, 2, inv.invoiceNo);
                setT("b2b", r, 3, xlsDate(inv.invoiceDate));
                setN("b2b", r, 4, inv.invoiceValue);
                setT("b2b", r, 5, Gstr1Engine::posDisplayName(inv.pos));
                setT("b2b", r, 6, inv.reverseCharge);
                setT("b2b", r, 8, invTypeDisplay(inv.invoiceType));
            }
            setN("b2b", r, 10, it.rate);
            setN("b2b", r, 11, it.taxableValue);
            setN("b2b", r, 12, it.cess);
        }
        res.stats.b2bRows += inv.items.size();
    }
    for (const auto& c : p.b2cl) {
        int r = newRow("b2cl", 9);
        setT("b2cl", r, 0, c.invoiceNo);
        setT("b2cl", r, 1, xlsDate(c.invoiceDate));
        setN("b2cl", r, 2, c.invoiceValue);
        setT("b2cl", r, 3, Gstr1Engine::posDisplayName(c.pos));
        setN("b2cl", r, 5, c.rate);
        setN("b2cl", r, 6, c.taxableValue);
        setN("b2cl", r, 7, c.cess);
        res.stats.b2clRows++;
    }
    for (const auto& cs : p.b2cs) {
        int r = newRow("b2cs", 7);
        setT("b2cs", r, 0, cs.typ);
        setT("b2cs", r, 1, Gstr1Engine::posDisplayName(cs.pos));
        setN("b2cs", r, 3, cs.rate);
        setN("b2cs", r, 4, cs.taxableValue);
        setN("b2cs", r, 5, cs.cess);
        res.stats.b2csRows++;
    }
    for (const auto& n : p.cdnr) {
        for (int li = 0; li < n.items.size(); li++) {
            const auto& it = n.items[li];
            int r = newRow("cdnr", 13);
            if (li == 0) {
                setT("cdnr", r, 0, n.ctin);
                setT("cdnr", r, 1, n.receiverName);
                setT("cdnr", r, 2, n.noteNo);
                setT("cdnr", r, 3, xlsDate(n.noteDate));
                setT("cdnr", r, 4, n.noteType);
                setT("cdnr", r, 5, Gstr1Engine::posDisplayName(n.pos));
                setT("cdnr", r, 6, n.reverseCharge);
                setT("cdnr", r, 7, invTypeDisplay(n.invType));
                setN("cdnr", r, 8, n.noteValue);
            }
            setN("cdnr", r, 10, it.rate);
            setN("cdnr", r, 11, it.taxableValue);
            setN("cdnr", r, 12, it.cess);
        }
        res.stats.cdnrRows += n.items.size();
    }
    for (const auto& n : p.cdnur) {
        for (int li = 0; li < n.items.size(); li++) {
            const auto& it = n.items[li];
            int r = newRow("cdnur", 10);
            if (li == 0) {
                setT("cdnur", r, 0, n.urType);
                setT("cdnur", r, 1, n.noteNo);
                setT("cdnur", r, 2, xlsDate(n.noteDate));
                setT("cdnur", r, 3, n.noteType);
                setT("cdnur", r, 4, Gstr1Engine::posDisplayName(n.pos));
                setN("cdnur", r, 5, n.noteValue);
            }
            setN("cdnur", r, 7, it.rate);
            setN("cdnur", r, 8, it.taxableValue);
            setN("cdnur", r, 9, it.cess);
        }
        res.stats.cdnurRows += n.items.size();
    }
    auto hsnRows = [&](const QString& sheet, const QList<Gstr1HsnItem>& list, int& counter) {
        for (const auto& h : list) {
            int r = newRow(sheet, 11);
            setT(sheet, r, 0, h.hsnCode);
            setT(sheet, r, 1, h.description);
            setT(sheet, r, 2, uqcDisplay(h.uqc));
            setN(sheet, r, 3, h.totalQty);
            setN(sheet, r, 4, h.totalValue);
            setN(sheet, r, 5, h.rate);
            setN(sheet, r, 6, h.taxableValue);
            setN(sheet, r, 7, h.igst);
            setN(sheet, r, 8, h.cgst);
            setN(sheet, r, 9, h.sgst);
            setN(sheet, r, 10, h.cess);
            counter++;
        }
    };
    hsnRows("hsn(b2b)", p.hsnB2B, res.stats.hsnB2BRows);
    hsnRows("hsn(b2c)", p.hsnB2C, res.stats.hsnB2CRows);
    for (const auto& d : p.docs) {
        int r = newRow("docs", 5);
        setT("docs", r, 0, d.docName);
        setT("docs", r, 1, d.fromSerial);
        setT("docs", r, 2, d.toSerial);
        setN("docs", r, 3, d.totalCount);
        setN("docs", r, 4, d.cancelledCount);
        res.stats.docRows++;
    }
    // exemp: fixed 4 rows from the payload (zeros when no exempt trade).
    for (const auto& e : p.exemp) {
        int r = newRow("exemp", 4);
        setT("exemp", r, 0, e.ttyp);
        setN("exemp", r, 1, e.nilAmt);
        setN("exemp", r, 2, e.exmpAmt);
        setN("exemp", r, 3, e.ngsupAmt);
    }

    // ---- Main ----
    QString fpYear = p.fp.length() == 6 ? p.fp.mid(2, 4) : "";
    QString fpMonth = p.fp.length() == 6 ? p.fp.left(2) : "";
    QString mainXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<dimension ref=\"A1:B6\"/>"
        "<sheetViews><sheetView workbookViewId=\"0\"/></sheetViews>"
        "<sheetFormat defaultRowHeight=\"15\"/><sheetData>"
        "<row r=\"1\">" + cellText("A1", "Ver 2.1", true) + "</row>"
        "<row r=\"2\">" + cellText("A2", "TAX PAYER DETAILS", true) + "</row>"
        "<row r=\"3\">" + cellText("A3", "Name of Tax Payer", true) + cellText("B3", p.legalName) + "</row>"
        "<row r=\"4\">" + cellText("A4", "GSTIN of the Tax Payer", true) + cellText("B4", p.gstin) + "</row>"
        "<row r=\"5\">" + cellText("A5", "Financial period (YYYY)", true) + cellText("B5", fpYear) + "</row>"
        "<row r=\"6\">" + cellText("A6", "Return month (MM)", true) + cellText("B6", fpMonth) + "</row>"
        "</sheetData></worksheet>";

    // ---- master ----
    QStringList masterKeys = {"UQC","EXP","RCHARGE","NOTE","TYPE","RATE","POS","INVTYPE","NOTERSN","DOCUMENT","URTYPE","STYPE","MONTH","FYEAR","DIFF","ECO_SUP"};
    QStringList masterTitles = {"UQC","Export Type","Reverse Charge/Provisional Assessment","Note Type","Type","Tax Rate","POS","Invoice Type","Reason For Issuing Note","Nature  of Document","UR Type","Supply Type","Month","Financial Year","Differential Percentage","Nature of Supply"};
    int masterRows = 1;
    for (const QString& k : masterKeys) masterRows = qMax(masterRows, masterCol(k).size() + 1);
    QString masterXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        + QString("<dimension ref=\"A1:P%1\"/>").arg(masterRows) +
        "<sheetViews><sheetView workbookViewId=\"0\"/></sheetViews>"
        "<sheetFormat defaultRowHeight=\"15\"/><sheetData><row r=\"1\">";
    for (int c = 0; c < masterTitles.size(); c++)
        masterXml += cellText(cellRef(1, c), masterTitles[c], true);
    masterXml += "</row>";
    for (int r = 2; r <= masterRows; r++) {
        masterXml += QString("<row r=\"%1\">").arg(r);
        for (int c = 0; c < masterKeys.size(); c++) {
            QStringList col = masterCol(masterKeys[c]);
            if (r - 2 < col.size()) {
                if (masterKeys[c] == "RATE") {
                    bool numOk = false;
                    double v = col[r - 2].toDouble(&numOk);
                    if (numOk) masterXml += cellNum(cellRef(r, c), rateNum(v));
                    else masterXml += cellText(cellRef(r, c), col[r - 2]);
                } else {
                    masterXml += cellText(cellRef(r, c), col[r - 2]);
                }
            }
        }
        masterXml += "</row>";
    }
    masterXml += "</sheetData></worksheet>";

    // ---- package ----
    struct Part { QString name; QString xml; };
    QVector<Part> parts;
    // order: Main + data sheets in template order, then master
    QStringList order = {"Main"};
    for (const SheetSpec& s : specs) order.append(s.name);
    order.append("master");
    QMap<QString, QString> xmlByName;
    xmlByName["Main"] = mainXml;
    xmlByName["master"] = masterXml;
    for (const SheetSpec& s : specs) {
        int cols = s.headers.size();
        xmlByName[s.name] = buildSheet(s, texts.value(s.name), nums.value(s.name), cols);
    }
    for (const QString& n : order) parts.append({n, xmlByName[n]});

    auto sheetFile = [&](int i) { return QString("xl/worksheets/sheet%1.xml").arg(i + 1); };

    QString contentTypes = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxml-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
        "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxml-package.core-properties+xml\"/>"
        "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>";
    for (int i = 0; i < parts.size(); i++)
        contentTypes += QString("<Override PartName=\"/%1\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>").arg(sheetFile(i));
    contentTypes += "</Types>";

    QString pkgRels = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
        "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
        "</Relationships>";

    QString wbRels = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";
    for (int i = 0; i < parts.size(); i++)
        wbRels += QString("<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet%1.xml\"/>").arg(i + 1);
    wbRels += QString("<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>").arg(parts.size() + 1);
    wbRels += "</Relationships>";

    QString workbook = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets>";
    for (int i = 0; i < parts.size(); i++)
        workbook += QString("<sheet name=\"%1\" sheetId=\"%2\" r:id=\"rId%2\"/>").arg(esc(parts[i].name)).arg(i + 1);
    workbook += "</sheets><definedNames>";
    auto defName = [&](const QString& n, const QString& col, int rows) {
        workbook += QString("<definedName name=\"%1\">master!$%2$2:$%2$%3</definedName>").arg(n, col).arg(rows + 1);
    };
    defName("UQC", "A", 44); defName("NUQC", "A", 44);
    defName("EXP", "B", 2);
    defName("RCHARGE", "C", 2);
    defName("NOTE", "D", 3); defName("CDRNOTE", "D", 2);
    defName("TYPE", "E", 2);
    defName("RATE", "F", 13);
    defName("POS", "G", 38);
    defName("INVTYPE", "H", 5);
    defName("NOTERSN", "I", 7);
    defName("DOCUMENT", "J", 12);
    defName("URTYPE", "K", 3);
    defName("STYPE", "L", 2);
    defName("MONTH", "M", 12);
    defName("FYEAR", "N", 12);
    defName("DIFF", "O", 1);
    defName("ECO_SUP", "P", 2);
    workbook += "</definedNames></workbook>";

    QString styles = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"2\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font>"
        "<font><b/><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
        "<fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill>"
        "<fill><patternFill patternType=\"gray125\"/></fill></fills>"
        "<borders count=\"1\"><border/></borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"2\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
        "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"0\" xfId=\"0\" applyFont=\"1\"/></cellXfs>"
        "</styleSheet>";

    // Fixed timestamps: byte-identical output for identical input.
    QString core = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:dcterms=\"http://purl.org/dc/terms/\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
        "<dc:creator>MahadevRiceMillERP</dc:creator><cp:lastModifiedBy>MahadevRiceMillERP</cp:lastModifiedBy>"
        "<dcterms:created xsi:type=\"dcterms:W3CDTF\">2025-01-01T00:00:00Z</dcterms:created>"
        "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">2025-01-01T00:00:00Z</dcterms:modified>"
        "</cp:coreProperties>";
    QString app = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
        "<Application>MahadevRiceMillERP GSTR-1</Application>"
        "</Properties>";

    mz_zip_archive outZip;
    memset(&outZip, 0, sizeof(outZip));
    if (!mz_zip_writer_init_heap(&outZip, 0, 0)) {
        res.error = "Could not initialize zip writer.";
        return res;
    }
    auto addPart = [&](const QString& name, const QString& xmlText) -> bool {
        QByteArray bytes = xmlText.toUtf8();
        return mz_zip_writer_add_mem(&outZip, name.toUtf8().constData(),
                                     bytes.constData(), bytes.size(), MZ_DEFAULT_COMPRESSION) != 0;
    };
    bool addOk = true;
    addOk = addOk && addPart("[Content_Types].xml", contentTypes);
    addOk = addOk && addPart("_rels/.rels", pkgRels);
    addOk = addOk && addPart("xl/workbook.xml", workbook);
    addOk = addOk && addPart("xl/_rels/workbook.xml.rels", wbRels);
    addOk = addOk && addPart("xl/styles.xml", styles);
    addOk = addOk && addPart("docProps/core.xml", core);
    addOk = addOk && addPart("docProps/app.xml", app);
    for (int i = 0; i < parts.size() && addOk; i++)
        addOk = addOk && addPart(sheetFile(i), parts[i].xml);
    if (!addOk) {
        mz_zip_writer_end(&outZip);
        res.error = "Could not add workbook parts.";
        return res;
    }
    void* pOutBuf = nullptr;
    size_t outSize = 0;
    if (!mz_zip_writer_finalize_heap_archive(&outZip, &pOutBuf, &outSize)) {
        mz_zip_writer_end(&outZip);
        res.error = "Could not finalize workbook.";
        return res;
    }
    QFileInfo outInfo(outPath);
    QDir().mkpath(outInfo.absolutePath());
    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        mz_free(pOutBuf);
        mz_zip_writer_end(&outZip);
        res.error = "Cannot write to: " + outPath;
        return res;
    }
    outFile.write((const char*)pOutBuf, (qint64)outSize);
    outFile.close();
    mz_free(pOutBuf);
    mz_zip_writer_end(&outZip);
    res.ok = true;
    return res;
}

} // namespace MahadevERP
