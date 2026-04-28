#include "exportmanager.h"
#include "database.h"
#include "clinicsettings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QTextDocument>
#include <QPrinter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QMap>
#include <QBuffer>
#include <QPixmap>
#include <QProcess>

// ═════════════════════════════════════════════════════════════════════════════
// CSV helpers
// ═════════════════════════════════════════════════════════════════════════════
static QString csvEscape(const QString& s)
{
    QString r = s;
    r.replace("\"", "\"\"");
    if (r.contains(',') || r.contains('"') || r.contains('\n'))
        r = "\"" + r + "\"";
    return r;
}
static void writeRow(QTextStream& ts, const QStringList& cols)
{
    QStringList esc;
    for (auto& c : cols) esc << csvEscape(c);
    ts << esc.join(",") << "\n";
}

// ═════════════════════════════════════════════════════════════════════════════
// HTML builder — moderan, profesionalan izgled
// ═════════════════════════════════════════════════════════════════════════════
static QString escape(const QString& s) {
    QString r = s;
    r.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;");
    return r;
}

static QString logoBase64Html()
{
    auto& cs = ClinicSettings::instance();
    if (!cs.hasLogo()) return QString();
    QPixmap px = cs.logo();
    if (px.isNull()) return QString();
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    px.scaled(160, 55, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buf, "PNG");
    buf.close();
    return "<img src='data:image/png;base64," + ba.toBase64() +
           "' style='max-height:55px;max-width:160px;vertical-align:middle;'/>";
}

static QString exportCss()
{
    return R"(
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body {
    font-family: 'Segoe UI', Arial, sans-serif;
    font-size: 9pt;
    color: #1A1A2E;
    background: white;
    padding: 16px;
}
/* ── Header ── */
.doc-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    border-bottom: 3px solid #16213E;
    padding-bottom: 10px;
    margin-bottom: 14px;
}
.clinic-name {
    font-size: 17pt;
    font-weight: 900;
    color: #16213E;
    letter-spacing: -0.5px;
}
.clinic-sub { font-size: 8pt; color: #777; margin-top: 2px; }
.clinic-contact { font-size: 7.5pt; color: #888; margin-top: 3px; }
.doc-meta { text-align: right; font-size: 7.5pt; color: #888; }
.doc-meta .doc-title {
    font-size: 11pt; font-weight: bold;
    color: #0F3460; margin-bottom: 3px;
}

/* ── Section titles ── */
h2 {
    font-size: 10pt;
    font-weight: bold;
    color: #0F3460;
    border-left: 4px solid #E94560;
    padding-left: 8px;
    margin: 12px 0 6px;
}

/* ── Patient card ── */
.patient-card {
    background: linear-gradient(135deg, #f8f9ff 0%, #eef1ff 100%);
    border: 1px solid #D0D9FF;
    border-radius: 8px;
    padding: 10px 14px;
    margin-bottom: 12px;
}
.patient-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 5px 12px;
}
.patient-field label {
    font-size: 7pt;
    color: #888;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    display: block;
}
.patient-field span {
    font-size: 9pt;
    font-weight: 600;
    color: #1A1A2E;
}
.patient-name-big {
    font-size: 14pt;
    font-weight: 900;
    color: #0F3460;
    margin-bottom: 8px;
}

/* ── Summary bar ── */
.summary-bar {
    display: flex;
    gap: 8px;
    margin-bottom: 12px;
}
.summary-chip {
    flex: 1;
    border-radius: 6px;
    padding: 7px 10px;
    text-align: center;
}
.chip-blue  { background: #EEF2FF; border: 1px solid #C7D2FE; }
.chip-green { background: #ECFDF5; border: 1px solid #A7F3D0; }
.chip-red   { background: #FFF1F2; border: 1px solid #FECDD3; }
.summary-chip .chip-val {
    font-size: 14pt; font-weight: 900; display: block;
}
.chip-blue .chip-val  { color: #4338CA; }
.chip-green .chip-val { color: #065F46; }
.chip-red .chip-val   { color: #9F1239; }
.summary-chip .chip-lbl {
    font-size: 7pt; color: #888;
    text-transform: uppercase; letter-spacing: 0.5px;
}

/* ── Interventions table ── */
table.iv-table {
    width: 100%;
    border-collapse: collapse;
    margin-bottom: 8px;
    font-size: 8pt;
}
table.iv-table thead tr {
    background: #16213E;
    color: white;
}
table.iv-table thead th {
    padding: 7px 8px;
    text-align: left;
    font-weight: 600;
    font-size: 7.5pt;
    letter-spacing: 0.3px;
    white-space: nowrap;
}
table.iv-table tbody tr:nth-child(even) {
    background: #F8F9FF;
}
table.iv-table tbody tr:hover {
    background: #EEF2FF;
}
table.iv-table td {
    padding: 6px 8px;
    border-bottom: 1px solid #EAECF8;
    vertical-align: top;
}
.badge {
    display: inline-block;
    padding: 2px 7px;
    border-radius: 10px;
    font-size: 7pt;
    font-weight: 700;
    white-space: nowrap;
}
.badge-done     { background: #D1FAE5; color: #065F46; }
.badge-sched    { background: #DBEAFE; color: #1E40AF; }
.badge-cancel   { background: #FEE2E2; color: #991B1B; }
.price-cell     { font-weight: 700; color: #0F3460; text-align: right; }
.tooth-cell     { font-weight: 600; color: #E94560; text-align: center; }
.type-cell      { font-weight: 600; }
.text-muted     { color: #888; font-size: 7.5pt; }

/* ── Full report patient row ── */
.pt-section {
    border: 1px solid #E2E8F0;
    border-radius: 6px;
    margin-bottom: 14px;
    page-break-inside: avoid;
    overflow: hidden;
}
.pt-section-header {
    background: #0F3460;
    color: white;
    padding: 7px 12px;
    font-weight: bold;
    font-size: 9.5pt;
    display: flex;
    justify-content: space-between;
    align-items: center;
}
.pt-section-header .pt-info {
    font-size: 7.5pt;
    color: #A0B4CC;
}

/* ── Footer ── */
.doc-footer {
    border-top: 1px solid #E2E8F0;
    margin-top: 16px;
    padding-top: 6px;
    font-size: 6.5pt;
    color: #AAA;
    text-align: center;
}
</style>
)";
}

static QString buildPatientSection(const Patient& p,
                                    const QList<Intervention>& ivs,
                                    bool showHeader = true)
{
    QString html;

    if (showHeader) {
        html += "<div class='patient-name-big'>👤 " + escape(p.fullName()) + "</div>";
        html += "<div class='patient-card'><div class='patient-grid'>";

        auto field = [&](const QString& lbl, const QString& val) {
            if (val.isEmpty()) return;
            html += "<div class='patient-field'>"
                    "<label>" + escape(lbl) + "</label>"
                    "<span>" + escape(val) + "</span>"
                    "</div>";
        };

        QDate dob = QDate::fromString(p.dateOfBirth, "yyyy-MM-dd");
        QString dobStr = dob.isValid() ? dob.toString("dd.MM.yyyy.") : p.dateOfBirth;
        int age = dob.isValid() ? dob.daysTo(QDate::currentDate()) / 365 : 0;

        field("Datum rođenja", dobStr + (age > 0 ? QString(" (%1 god.)").arg(age) : ""));
        field("Pol",           p.gender);
        field("Telefon",       p.phone);
        field("E-mail",        p.email);
        field("Adresa",        p.address);
        field("Alergije",      p.allergies);
        field("Matični broj",  p.nationalId);
        if (!p.notes.isEmpty())
            field("Napomene", p.notes);

        html += "</div></div>";
    }

    // Summary chips
    int done = 0, scheduled = 0, cancelled = 0;
    double total = 0;
    QString currency = "BAM";
    for (const auto& iv : ivs) {
        if (iv.status == "completed")  { done++; total += iv.cost; currency = iv.currency; }
        else if (iv.status == "scheduled")  scheduled++;
        else cancelled++;
    }

    html += "<div class='summary-bar'>"
            "<div class='summary-chip chip-blue'>"
            "<span class='chip-val'>" + QString::number(ivs.size()) + "</span>"
            "<span class='chip-lbl'>Ukupno intervencija</span></div>"
            "<div class='summary-chip chip-green'>"
            "<span class='chip-val'>" + QString::number(total,'f',2) + " " + escape(currency) + "</span>"
            "<span class='chip-lbl'>Ukupno naplaćeno</span></div>"
            "<div class='summary-chip chip-red'>"
            "<span class='chip-val'>" + QString::number(scheduled) + "</span>"
            "<span class='chip-lbl'>Zakazano</span></div>"
            "</div>";

    if (ivs.isEmpty()) {
        html += "<p class='text-muted'>Nema intervencija.</p>";
        return html;
    }

    // Table
    html += "<table class='iv-table'><thead><tr>"
            "<th>Datum</th>"
            "<th>Intervencija</th>"
            "<th>Zub</th>"
            "<th>Doktor</th>"
            "<th>Proizvod</th>"
            "<th>Dijagnoza / Terapija</th>"
            "<th>Napomene</th>"
            "<th style='text-align:right;'>Cijena</th>"
            "<th>Status</th>"
            "</tr></thead><tbody>";

    for (const auto& iv : ivs) {
        QString dt = iv.dateTime;
        if (dt.length() >= 16)
            dt = QDateTime::fromString(iv.dateTime, "yyyy-MM-dd HH:mm")
                     .toString("dd.MM.yyyy\nHH:mm");
        else if (dt.length() >= 10)
            dt = QDate::fromString(iv.dateTime.left(10), "yyyy-MM-dd")
                     .toString("dd.MM.yyyy.");

        QString badge, badgeCls;
        if (iv.status == "completed") { badge = "Završeno"; badgeCls = "badge-done"; }
        else if (iv.status == "scheduled") { badge = "Zakazano"; badgeCls = "badge-sched"; }
        else { badge = "Otkazano"; badgeCls = "badge-cancel"; }

        QString diagTreat;
        if (!iv.diagnosis.isEmpty())
            diagTreat += "<b>Dijagnoza:</b> " + escape(iv.diagnosis) + "<br/>";
        if (!iv.treatment.isEmpty())
            diagTreat += "<b>Terapija:</b> " + escape(iv.treatment);
        if (diagTreat.isEmpty() && !iv.description.isEmpty())
            diagTreat = escape(iv.description);

        html += "<tr>"
                "<td class='text-muted'>" + escape(dt).replace("\n","<br/>") + "</td>"
                "<td class='type-cell'>" + escape(iv.interventionType) + "</td>"
                "<td class='tooth-cell'>" + escape(iv.tooth) + "</td>"
                "<td>" + escape(iv.doctor) + "</td>"
                "<td class='text-muted'>" + escape(iv.product) + "</td>"
                "<td>" + diagTreat + "</td>"
                "<td class='text-muted'>" + escape(iv.notes) + "</td>"
                "<td class='price-cell'>" + QString::number(iv.cost,'f',2) + "<br/>"
                "<span class='text-muted'>" + escape(iv.currency) + "</span></td>"
                "<td><span class='badge " + badgeCls + "'>" + badge + "</span></td>"
                "</tr>";
    }

    // Total row
    html += "<tr style='background:#F0F4FF;font-weight:bold;'>"
            "<td colspan='7' style='text-align:right;padding:7px 8px;"
            "color:#0F3460;'>UKUPNO NAPLAĆENO:</td>"
            "<td class='price-cell' style='font-size:10pt;color:#065F46;'>"
            + QString::number(total,'f',2) + "<br/>"
            "<span style='font-size:7pt;'>" + escape(currency) + "</span></td>"
            "<td></td></tr>";

    html += "</tbody></table>";
    return html;
}

static QString buildDocHeader(const QString& docTitle)
{
    auto& cs = ClinicSettings::instance();
    QStringList contact;
    if (!cs.clinicAddress.isEmpty()) contact << cs.clinicAddress;
    if (!cs.clinicPhone.isEmpty())   contact << cs.clinicPhone;
    if (!cs.clinicEmail.isEmpty())   contact << cs.clinicEmail;

    return "<div class='doc-header'>"
           "<div style='display:flex;align-items:center;gap:12px;'>"
           + logoBase64Html() +
           "<div>"
           "<div class='clinic-name'>" + escape(cs.clinicName) + "</div>"
           "<div class='clinic-sub'>" + escape(cs.clinicSubtitle) + "</div>"
           "<div class='clinic-contact'>" + escape(contact.join("  ·  ")) + "</div>"
           "</div></div>"
           "<div class='doc-meta'>"
           "<div class='doc-title'>" + escape(docTitle) + "</div>"
           "<div>Datum: " + QDate::currentDate().toString("dd.MM.yyyy.") + "</div>"
           "<div>Vrijemme: " + QTime::currentTime().toString("HH:mm") + "</div>"
           "</div></div>";
}

static QString buildDocFooter()
{
    auto& cs = ClinicSettings::instance();
    return "<div class='doc-footer'>"
           + escape(cs.clinicName) +
           " — Povjerljivi medicinski dokument — "
           "Generisano: " + QDateTime::currentDateTime().toString("dd.MM.yyyy. HH:mm") +
           "</div>";
}

// ═════════════════════════════════════════════════════════════════════════════
// Shared HTML builders
// ═════════════════════════════════════════════════════════════════════════════
QString ExportManager::buildExportHtml(const Patient& p,
                                        const QList<Intervention>& ivs,
                                        bool /*singlePatient*/)
{
    QString html = "<html><head><meta charset='utf-8'/>"
                   + exportCss() + "</head><body>";
    html += buildDocHeader("Karton pacijenta");
    html += "<h2>Podaci o pacijentu</h2>";
    html += buildPatientSection(p, ivs, true);
    html += buildDocFooter();
    html += "</body></html>";
    return html;
}

QString ExportManager::buildFullReportHtml(
    const QList<Patient>& patients,
    const QMap<int, QList<Intervention>>& ivMap)
{
    auto& cs = ClinicSettings::instance();
    QString html = "<html><head><meta charset='utf-8'/>"
                   + exportCss() + "</head><body>";
    html += buildDocHeader("Kompletan izvještaj ordinacije");

    // Global summary
    int totalPat = patients.size();
    int totalIv  = 0;
    double totalRev = 0;
    QString cur = "BAM";
    for (const auto& p : patients) {
        const auto& ivs = ivMap.value(p.id);
        for (const auto& iv : ivs) {
            totalIv++;
            if (iv.status == "completed") { totalRev += iv.cost; cur = iv.currency; }
        }
    }

    html += "<div class='summary-bar' style='margin-bottom:16px;'>"
            "<div class='summary-chip chip-blue'>"
            "<span class='chip-val'>" + QString::number(totalPat) + "</span>"
            "<span class='chip-lbl'>Pacijenata</span></div>"
            "<div class='summary-chip chip-green'>"
            "<span class='chip-val'>" + QString::number(totalIv) + "</span>"
            "<span class='chip-lbl'>Intervencija</span></div>"
            "<div class='summary-chip chip-red'>"
            "<span class='chip-val'>" + QString::number(totalRev,'f',2) + " " + escape(cur) + "</span>"
            "<span class='chip-lbl'>Ukupno naplaćeno</span></div>"
            "</div>";

    // Each patient
    for (const auto& p : patients) {
        const auto& ivs = ivMap.value(p.id);
        if (ivs.isEmpty()) continue;

        int done = 0; double ptTotal = 0;
        for (const auto& iv : ivs)
            if (iv.status == "completed") { done++; ptTotal += iv.cost; }

        html += "<div class='pt-section'>"
                "<div class='pt-section-header'>"
                "<span>👤 " + escape(p.fullName()) + "</span>"
                "<span class='pt-info'>"
                + (!p.dateOfBirth.isEmpty()
                    ? QDate::fromString(p.dateOfBirth,"yyyy-MM-dd").toString("dd.MM.yyyy.") + "  ·  "
                    : "") +
                (!p.phone.isEmpty() ? p.phone + "  ·  " : "") +
                QString::number(ivs.size()) + " intervencija  ·  " +
                QString::number(ptTotal,'f',2) + " " + cur +
                "</span></div>"
                "<div style='padding:8px 12px;'>";
        html += buildPatientSection(p, ivs, false);
        html += "</div></div>";
    }

    html += buildDocFooter();
    html += "</body></html>";
    return html;
}

// ═════════════════════════════════════════════════════════════════════════════
// PDF export
// ═════════════════════════════════════════════════════════════════════════════
bool ExportManager::exportPatientToPdf(const Patient& p,
                                        const QList<Intervention>& ivs,
                                        QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/" + p.fullName().replace(" ","_") + "_karton.pdf";

    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi karton pacijenta u PDF", def,
        "PDF dokumenti (*.pdf)");
    if (path.isEmpty()) return false;

    QTextDocument doc;
    doc.setHtml(buildExportHtml(p, ivs));

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(12,12,12,12), QPageLayout::Millimeter);
    printer.setPageOrientation(QPageLayout::Landscape);

    doc.print(&printer);

    QMessageBox::information(parent, "PDF izvoz uspješan",
        QString("Karton pacijenta sačuvan:\n%1").arg(path));
    return true;
}

bool ExportManager::exportFullReportToPdf(QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/Ordinacija_izvjestaj_"
                  + QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";

    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi kompletan izvještaj u PDF", def,
        "PDF dokumenti (*.pdf)");
    if (path.isEmpty()) return false;

    auto patients = Database::instance().getAllPatients();
    QMap<int, QList<Intervention>> ivMap;
    for (const auto& p : patients)
        ivMap[p.id] = Database::instance().getPatientInterventions(p.id);

    QTextDocument doc;
    doc.setHtml(buildFullReportHtml(patients, ivMap));

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(10,10,10,10), QPageLayout::Millimeter);
    printer.setPageOrientation(QPageLayout::Landscape);

    doc.print(&printer);

    QMessageBox::information(parent, "PDF izvoz uspješan",
        QString("Kompletan izvještaj sačuvan:\n%1\n\n%2 pacijenata.")
            .arg(path).arg(patients.size()));
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// Excel XLSX export (proper .xlsx format)
// ═════════════════════════════════════════════════════════════════════════════
// Generišemo .xlsx kao ZIP sa XML fajlovima (Open XML standard)
static void writeXlsxFile(const QString& path,
                           const QString& sheetTitle,
                           const QList<QStringList>& rows,
                           const QStringList& header,
                           const QString& clinicName)
{
    // Collect all string values for shared strings
    QStringList strings;
    auto addStr = [&](const QString& s) -> int {
        int i = strings.indexOf(s);
        if (i < 0) { i = strings.size(); strings << s; }
        return i;
    };

    // Pre-build header indices
    QList<int> headerIdx;
    for (const auto& h : header) headerIdx << addStr(h);

    QList<QList<int>> rowIdx;
    for (const auto& row : rows) {
        QList<int> ri;
        for (const auto& cell : row) ri << addStr(cell);
        rowIdx << ri;
    }

    // Build sheet XML
    auto colName = [](int col) -> QString {
        QString r;
        col++;
        while (col > 0) {
            r.prepend(QChar('A' + (col-1) % 26));
            col = (col-1) / 26;
        }
        return r;
    };

    QString sheetXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData>";

    // Header row
    sheetXml += "<row r=\"1\">";
    for (int c = 0; c < headerIdx.size(); ++c) {
        sheetXml += QString("<c r=\"%1%2\" t=\"s\" s=\"1\">"
                            "<v>%3</v></c>")
                    .arg(colName(c)).arg(1).arg(headerIdx[c]);
    }
    sheetXml += "</row>";

    // Data rows
    for (int r = 0; r < rowIdx.size(); ++r) {
        sheetXml += QString("<row r=\"%1\">").arg(r+2);
        const auto& ri = rowIdx[r];
        for (int c = 0; c < ri.size(); ++c) {
            sheetXml += QString("<c r=\"%1%2\" t=\"s\" s=\"2\">"
                                "<v>%3</v></c>")
                        .arg(colName(c)).arg(r+2).arg(ri[c]);
        }
        sheetXml += "</row>";
    }
    sheetXml += "</sheetData></worksheet>";

    // Shared strings XML
    QString ssXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
        " count=\"" + QString::number(strings.size()) + "\""
        " uniqueCount=\"" + QString::number(strings.size()) + "\">";
    for (const auto& s : strings) {
        QString escaped = s;
        escaped.replace("&","&amp;").replace("<","&lt;")
               .replace(">","&gt;").replace("\"","&quot;");
        ssXml += "<si><t xml:space=\"preserve\">" + escaped + "</t></si>";
    }
    ssXml += "</sst>";

    // Styles XML — header bold/blue, data normal
    QString stylesXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
<fonts><font><sz val="9"/><name val="Segoe UI"/></font>
<font><sz val="9"/><b/><color rgb="FFFFFFFF"/><name val="Segoe UI"/></font></fonts>
<fills>
<fill><patternFill patternType="none"/></fill>
<fill><patternFill patternType="gray125"/></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FF16213E"/></patternFill></fill>
<fill><patternFill patternType="solid"><fgColor rgb="FFF8F9FF"/></patternFill></fill>
</fills>
<borders><border><left/><right/><top/><bottom/><diagonal/></border>
<border><left style="thin"><color rgb="FFD0D9FF"/></left>
<right style="thin"><color rgb="FFD0D9FF"/></right>
<top style="thin"><color rgb="FFD0D9FF"/></top>
<bottom style="thin"><color rgb="FFD0D9FF"/></bottom></border>
</borders>
<cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>
<cellXfs>
<xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/>
<xf numFmtId="0" fontId="1" fillId="2" borderId="1" xfId="0" applyFont="1" applyFill="1" applyBorder="1"/>
<xf numFmtId="0" fontId="0" fillId="3" borderId="1" xfId="0" applyFill="1" applyBorder="1"/>
</cellXfs>
</styleSheet>)";

    // Workbook XML
    QString wbXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"
xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
<sheets><sheet name=")" + sheetTitle.left(31) + R"(" sheetId="1" r:id="rId1"/></sheets>
</workbook>)";

    // Relationships
    QString wbRels = R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings" Target="sharedStrings.xml"/>
<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)";

    QString rootRels = R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)";

    QString contentTypes = R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
<Default Extension="xml" ContentType="application/xml"/>
<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
<Override PartName="/xl/sharedStrings.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml"/>
<Override PartName="/xl/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>
</Types>)";

    // Write all to a temp dir then zip
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                     + "/dentapro_xlsx_tmp";
    QDir(tmpDir).removeRecursively();
    QDir().mkpath(tmpDir + "/_rels");
    QDir().mkpath(tmpDir + "/xl/_rels");
    QDir().mkpath(tmpDir + "/xl/worksheets");

    auto writeFile = [&](const QString& relPath, const QString& content) {
        QFile f(tmpDir + "/" + relPath);
        f.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);
        ts << content;
        f.close();
    };

    writeFile("[Content_Types].xml", contentTypes);
    writeFile("_rels/.rels",                rootRels);
    writeFile("xl/workbook.xml",             wbXml);
    writeFile("xl/_rels/workbook.xml.rels",  wbRels);
    writeFile("xl/worksheets/sheet1.xml",    sheetXml);
    writeFile("xl/sharedStrings.xml",        ssXml);
    writeFile("xl/styles.xml",               stylesXml);

    // Zip the folder into .xlsx using Qt
    if (QFile::exists(path)) QFile::remove(path);

    // Use system zip or Qt zip
    QStringList args;
    args << "-r" << path << ".";
    QProcess proc;
    proc.setWorkingDirectory(tmpDir);
    proc.start("zip", args);
    proc.waitForFinished(30000);

    QDir(tmpDir).removeRecursively();
}

bool ExportManager::exportPatientToXlsx(const Patient& p,
                                          const QList<Intervention>& ivs,
                                          QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/" + p.fullName().replace(" ","_") + "_karton.xlsx";

    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi karton pacijenta u Excel", def,
        "Excel datoteke (*.xlsx)");
    if (path.isEmpty()) return false;

    QStringList header = {"Datum", "Intervencija", "Zub", "Doktor",
                          "Proizvod", "Dijagnoza", "Terapija",
                          "Opis rada", "Napomene", "Cijena",
                          "Valuta", "Status"};

    QList<QStringList> rows;
    for (const auto& iv : ivs) {
        QString dt = iv.dateTime.length() >= 10
            ? QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd").toString("dd.MM.yyyy.")
            : iv.dateTime;
        QString status = iv.status == "completed" ? "Završeno" :
                         iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
        rows << QStringList{dt, iv.interventionType, iv.tooth, iv.doctor,
                            iv.product, iv.diagnosis, iv.treatment,
                            iv.description, iv.notes,
                            QString::number(iv.cost,'f',2),
                            iv.currency, status};
    }

    writeXlsxFile(path, p.fullName().left(31), rows, header,
                  ClinicSettings::instance().clinicName);

    if (QFile::exists(path)) {
        QMessageBox::information(parent, "Excel izvoz uspješan",
            QString("Karton pacijenta sačuvan:\n%1").arg(path));
        return true;
    }

    // Fallback — save as CSV if zip not available
    return exportPatientToCsv(p, ivs, parent);
}

bool ExportManager::exportFullReportToXlsx(QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/Ordinacija_izvjestaj_"
                  + QDate::currentDate().toString("yyyy-MM-dd") + ".xlsx";

    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi kompletan izvještaj u Excel", def,
        "Excel datoteke (*.xlsx)");
    if (path.isEmpty()) return false;

    QStringList header = {"Pacijent", "Datum rodjenja", "Telefon",
                          "Datum int.", "Intervencija", "Zub",
                          "Doktor", "Proizvod", "Dijagnoza", "Terapija",
                          "Napomene", "Cijena", "Valuta", "Status"};

    QList<QStringList> rows;
    auto patients = Database::instance().getAllPatients();
    for (const auto& p : patients) {
        auto ivs = Database::instance().getPatientInterventions(p.id);
        for (const auto& iv : ivs) {
            QString dt = iv.dateTime.length() >= 10
                ? QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd").toString("dd.MM.yyyy.")
                : iv.dateTime;
            QString status = iv.status == "completed" ? "Završeno" :
                             iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
            rows << QStringList{p.fullName(), p.dateOfBirth, p.phone,
                                dt, iv.interventionType, iv.tooth,
                                iv.doctor, iv.product, iv.diagnosis,
                                iv.treatment, iv.notes,
                                QString::number(iv.cost,'f',2),
                                iv.currency, status};
        }
    }

    writeXlsxFile(path, QString("Izvjestaj"), rows, header,
                  ClinicSettings::instance().clinicName);

    if (QFile::exists(path)) {
        QMessageBox::information(parent, "Excel izvoz uspješan",
            QString("Kompletan izvještaj sačuvan:\n%1\n\n%2 pacijenata.")
                .arg(path).arg(patients.size()));
        return true;
    }

    return exportFullReportToCsv(parent);
}

// ═════════════════════════════════════════════════════════════════════════════
// CSV (unchanged)
// ═════════════════════════════════════════════════════════════════════════════
bool ExportManager::exportPatientToCsv(const Patient& p,
                                         const QList<Intervention>& ivs,
                                         QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/" + p.fullName().replace(" ","_") + "_intervencije.csv";
    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi u CSV", def, "CSV fajlovi (*.csv)");
    if (path.isEmpty()) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Greška", "Nije moguće kreirati fajl:\n" + path);
        return false;
    }
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << "\xEF\xBB\xBF";

    auto& cs = ClinicSettings::instance();
    ts << "# " << cs.clinicName << "\n# Izvoz: "
       << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm") << "\n#\n";

    writeRow(ts, {p.fullName(), p.dateOfBirth, p.phone});
    ts << "#\n";
    writeRow(ts, {"Datum","Vrsta intervencije","Zub","Dijagnoza",
                  "Tretman","Doktor","Cijena","Valuta","Status","Napomene"});
    double ukupno = 0;
    for (const auto& iv : ivs) {
        QString datum = iv.dateTime.length() >= 10
            ? QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd").toString("dd.MM.yyyy.")
            : iv.dateTime;
        QString status = iv.status == "completed" ? "Zavrseno" :
                         iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
        writeRow(ts, {datum, iv.interventionType, iv.tooth,
                      iv.diagnosis, iv.treatment, iv.doctor,
                      QString::number(iv.cost,'f',2), iv.currency, status, iv.notes});
        if (iv.status == "completed") ukupno += iv.cost;
    }
    ts << "#\n";
    writeRow(ts, {"UKUPNO","","","","","",QString::number(ukupno,'f',2),"BAM","",""});
    f.close();

    QMessageBox::information(parent, "Izvoz uspješan",
        QString("Fajl sačuvan:\n%1").arg(path));
    return true;
}

bool ExportManager::exportFullReportToCsv(QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/DentaPro_izvjestaj_"
                  + QDate::currentDate().toString("yyyy-MM-dd") + ".csv";
    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi kompletan izvještaj", def, "CSV fajlovi (*.csv)");
    if (path.isEmpty()) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Greška", "Nije moguće kreirati fajl.");
        return false;
    }
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << "\xEF\xBB\xBF";

    auto& cs = ClinicSettings::instance();
    ts << "# " << cs.clinicName << " - Kompletan izvjestaj\n";
    ts << "# Datum: " << QDate::currentDate().toString("dd.MM.yyyy.") << "\n#\n";

    writeRow(ts, {"Pacijent","Datum rodjenja","Telefon",
                  "Datum int.","Vrsta","Zub","Doktor","Cijena (BAM)","Status"});

    auto patients = Database::instance().getAllPatients();
    double grandTotal = 0;
    for (const auto& p : patients) {
        auto ivs = Database::instance().getPatientInterventions(p.id);
        for (const auto& iv : ivs) {
            QString datum = iv.dateTime.length() >= 10
                ? QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd").toString("dd.MM.yyyy.")
                : iv.dateTime;
            QString status = iv.status == "completed" ? "Zavrseno" :
                             iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
            writeRow(ts, {p.fullName(), p.dateOfBirth, p.phone,
                          datum, iv.interventionType, iv.tooth,
                          iv.doctor, QString::number(iv.cost,'f',2), status});
            if (iv.status == "completed") grandTotal += iv.cost;
        }
    }
    ts << "#\n";
    writeRow(ts, {"UKUPNI PRIHOD","","","","","","",
                  QString::number(grandTotal,'f',2),""});
    f.close();

    QMessageBox::information(parent, "Izvoz uspješan",
        QString("Kompletan izvještaj sačuvan:\n%1").arg(path));
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// Backup (unchanged)
// ═════════════════════════════════════════════════════════════════════════════
bool ExportManager::backupDatabase(QWidget* parent)
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + "/dentapro.db";
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/DentaPro_backup_"
                  + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm") + ".db";
    QString dest = QFileDialog::getSaveFileName(
        parent, "Sačuvaj backup baze podataka", def,
        "Baza podataka (*.db)");
    if (dest.isEmpty()) return false;
    if (QFile::exists(dest)) QFile::remove(dest);
    if (QFile::copy(dbPath, dest)) {
        QMessageBox::information(parent, "Backup uspješan",
            QString("Backup sačuvan:\n%1\n\nCuvajte na sigurnom (USB, cloud).").arg(dest));
        return true;
    }
    QMessageBox::critical(parent, "Greška", "Backup nije uspio.");
    return false;
}

void ExportManager::autoBackup()
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + "/dentapro.db";
    if (!QFile::exists(dbPath)) return;
    QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                        + "/backups";
    QDir().mkpath(backupDir);
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString dest  = backupDir + "/dentapro_" + today + ".db";
    if (!QFile::exists(dest)) {
        QFile::copy(dbPath, dest);
        QDir d(backupDir);
        auto entries = d.entryList({"dentapro_*.db"}, QDir::Files, QDir::Name);
        while (entries.size() > 7) {
            d.remove(entries.first());
            entries.removeFirst();
        }
    }
}
