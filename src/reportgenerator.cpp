#include "reportgenerator.h"
#include "clinicsettings.h"
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QWidget>
#include <QDate>
#include <QDateTime>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QBuffer>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>

#ifdef HAVE_QT_PDF
#include <QPdfDocument>
#endif

QString ReportGenerator::pdfToHtmlImages(const QString& pdfPath, int maxWidthPx)
{
    if (pdfPath.isEmpty() || !QFile::exists(pdfPath)) return QString();

#ifdef HAVE_QT_PDF
    QPdfDocument doc;
    if (doc.load(pdfPath) != QPdfDocument::Error::None) return QString();

    QString html;
    QFileInfo fi(pdfPath);
    html += QString("<div style='margin-top:10px;border-top:2px solid #1565C0;"
                    "padding-top:6px;page-break-before:always;'>"
                    "<div style='font-weight:bold;color:#1565C0;font-size:8pt;"
                    "margin-bottom:4px;'>📄 %1</div>").arg(fi.fileName());

    int pageCount = doc.pageCount();
    for (int i = 0; i < pageCount; ++i) {
        QSizeF pageSize = doc.pagePointSize(i);
        double scale    = maxWidthPx / pageSize.width();
        QSize renderSize(maxWidthPx, qRound(pageSize.height() * scale));

        // Qt 6.5 API: QPdfDocument::render(page, imageSize)
        QImage img = doc.render(i, renderSize);
        if (img.isNull()) continue;

        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        img.save(&buf, "PNG");
        buf.close();

        if (pageCount > 1)
            html += QString("<div style='font-size:7pt;color:#9E9E9E;"
                            "margin-bottom:2px;'>Stranica %1 od %2</div>")
                    .arg(i+1).arg(pageCount);

        html += QString("<img src='data:image/png;base64,%1' "
                        "style='width:100%;border:1px solid #E0E0E0;"
                        "border-radius:3px;margin-bottom:6px;display:block;'/>\n")
                .arg(QString(ba.toBase64()));
    }
    html += "</div>";
    return html;

#else
    // Fallback — samo naziv fajla
    QFileInfo fi(pdfPath);
    return QString("<div style='margin-top:8px;padding:6px 10px;"
                   "background:#E3F2FD;border-left:3px solid #1565C0;"
                   "border-radius:0 4px 4px 0;font-size:8pt;color:#1565C0;'>"
                   "📄 <b>%1</b></div>").arg(fi.fileName());
#endif
}

static QString escape(const QString& s) {
    QString r = s;
    r.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;");
    return r;
}

// Embed image file as base64 za stampu
static QString imageToBase64Html(const QString& path,
                                  int maxW = 250, int maxH = 120)
{
    if (path.isEmpty() || !QFile::exists(path)) return QString();
    QPixmap px(path);
    if (px.isNull()) return QString();
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    px.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation)
      .save(&buf, "PNG");
    buf.close();
    return "<img src='data:image/png;base64," + ba.toBase64() +
           "' style='max-width:" + QString::number(maxW) + "px;"
           "max-height:" + QString::number(maxH) + "px;"
           "border:1px solid #E0E0E0;border-radius:4px;margin-top:4px;'/>";
}

static QString cssBase()
{
    return R"(
    <style>
    * { margin:0; padding:0; box-sizing:border-box; }
    body { font-family:'Segoe UI',Arial,sans-serif; font-size:8pt; color:#212121;
           background:white; padding:8px; }
    .header { border-bottom:2px solid #1565C0; padding-bottom:5px; margin-bottom:7px;
              display:flex; justify-content:space-between; }
    .clinic-name { font-size:13pt; font-weight:bold; color:#1565C0; }
    .clinic-sub  { font-size:7pt; color:#757575; margin-top:2px; }
    .print-date  { font-size:7pt; color:#9E9E9E; text-align:right; }
    h2 { font-size:9pt; color:#1565C0; margin:5px 0 3px;
         border-bottom:1px solid #E0E0E0; padding-bottom:2px; }
    h3 { font-size:8.5pt; color:#00695C; margin:4px 0 3px; }
    .info-table  { width:100%; border-collapse:collapse; margin-bottom:5px; }
    .info-table td { padding:2px 5px; vertical-align:top; font-size:7.5pt; }
    .info-table td:first-child { font-weight:bold; color:#555; width:30%; }
    .info-table tr:nth-child(even) td { background:#F9F9F9; }
    .iv-card { border:1px solid #E0E0E0; border-radius:4px; margin-bottom:5px;
               page-break-inside:avoid; }
    .iv-header { background:#1565C0; color:white; padding:4px 8px;
                 border-radius:3px 3px 0 0; font-weight:bold; font-size:8.5pt; }
    .iv-body   { padding:5px 8px; }
    .iv-body table { width:100%; border-collapse:collapse; }
    .iv-body td { padding:2px 4px; vertical-align:top; }
    .iv-body td:first-child { font-weight:bold; color:#555; width:30%; }
    .iv-body tr:nth-child(even) td { background:#F9F9F9; }
    .cost-badge { display:inline-block; background:#FFF9C4; border:1px solid #F9A825;
                  border-radius:3px; padding:1px 5px; font-weight:bold; color:#F57F17; }
    .status-completed { color:#2E7D32; font-weight:bold; }
    .status-scheduled { color:#1565C0; font-weight:bold; }
    .status-cancelled { color:#C62828; font-weight:bold; }
    .pdf-note { background:#E3F2FD; border-left:2px solid #1565C0; padding:3px 7px;
                margin-top:4px; font-size:7.5pt; color:#1565C0; }
    .consent-note { background:#F3E5F5; border-left:2px solid #6A1B9A;
                    padding:3px 7px; margin-top:4px; font-size:7.5pt; color:#6A1B9A; }
    .sig-section { margin-top:3px; padding:4px;
                   background:#FAFAFA; border:1px solid #E0E0E0; border-radius:3px; }
    .sig-line { border-bottom:1px solid #424242; width:160px; margin:12px 0 2px; }
    .footer { border-top:1px solid #E0E0E0; margin-top:6px; padding-top:3px;
              font-size:6.5pt; color:#9E9E9E; text-align:center; }
    .summary-box { border:1px solid #C8E6C9; background:#F1F8E9; border-radius:4px;
                   padding:4px 8px; margin-bottom:5px; }
    .summary-box td { padding:1px 8px 1px 0; font-size:7.5pt; }
    .summary-box td:first-child { font-weight:bold; color:#33691E; }
    </style>
    )";
}

static QString headerHtml()
{
    auto& cs = ClinicSettings::instance();

    QString logoHtml;
    if (cs.hasLogo()) {
        QPixmap px = cs.logo();
        if (!px.isNull()) {
            QByteArray ba;
            QBuffer buf(&ba);
            buf.open(QIODevice::WriteOnly);
            px.scaled(180, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)
              .save(&buf, "PNG");
            buf.close();
            logoHtml = "<img src='data:image/png;base64," + ba.toBase64() +
                       "' style='max-height:60px;max-width:180px;"
                       "vertical-align:middle;'/>";
        }
    }

    QString contactLine;
    QStringList parts;
    if (!cs.clinicAddress.isEmpty()) parts << cs.clinicAddress;
    if (!cs.clinicPhone.isEmpty())   parts << "Tel: " + cs.clinicPhone;
    if (!cs.clinicEmail.isEmpty())   parts << cs.clinicEmail;
    if (!parts.isEmpty())
        contactLine = "<div style='font-size:8pt;color:#757575;margin-top:2px;'>"
                      + escape(parts.join("  |  ")) + "</div>";

    return QString(R"(
    <div class='header'>
      <div style='display:flex;align-items:center;gap:12px;'>
        %1
        <div>
          <div class='clinic-name'>%2</div>
          <div class='clinic-sub'>%3</div>
          %4
        </div>
      </div>
      <div class='print-date'>
        Datum ispisa:<br/><b>%5</b>
      </div>
    </div>
    )").arg(logoHtml,
            escape(cs.clinicName),
            escape(cs.clinicSubtitle),
            contactLine,
            QDate::currentDate().toString("dd.MM.yyyy"));
}

static QString footerHtml()
{
    auto& cs = ClinicSettings::instance();
    QString naziv = cs.clinicName.isEmpty() ? "Zubarska ordinacija" : cs.clinicName;
    return "<div class='footer'>"
           + escape(naziv) +
           " — Povjerljivi medicinski dokument"
           "</div>";
}

// Pomocna za prikaz potpisa u HTML-u
static QString signatureHtml(const QString& path, const QString& label)
{
    if (path.isEmpty() || !QFile::exists(path)) return QString();
    QString imgHtml = imageToBase64Html(path, 280, 100);
    if (imgHtml.isEmpty()) return QString();
    return "<tr><td colspan='2'><div class='sig-section'>"
           "<b>" + escape(label) + ":</b><br/>" +
           imgHtml +
           "</div></td></tr>";
}

QString ReportGenerator::buildPatientReportHtml(const Patient& patient,
                                                 const QList<Intervention>& interventions,
                                                 bool includePdfs,
                                                 bool includeConsent,
                                                 bool includeHealth,
                                                 bool includePhotos,
                                                 bool hidePrice)
{
    QString html = "<html><head>" + cssBase() + "</head><body>";
    html += headerHtml();

    // Patient info
    html += "<h2>👤 Podaci o pacijentu</h2>";
    html += "<div class='summary-box'><table>";
    auto info = [&](const QString& lbl, const QString& val) {
        if (!val.isEmpty())
            html += "<tr><td>" + escape(lbl) + ":</td><td><b>" + escape(val) + "</b></td></tr>";
    };
    info("Ime i prezime", patient.fullName());
    info("Datum rođenja", patient.dateOfBirth.isEmpty() ? "" :
         QDate::fromString(patient.dateOfBirth,"yyyy-MM-dd").toString("dd.MM.yyyy."));
    info("Pol", patient.gender);
    info("Telefon", patient.phone);
    info("E-mail", patient.email);
    info("Adresa", patient.address);
    info("Alergije",     patient.allergies);
    info("Matični broj", patient.nationalId);
    html += "</table></div>";

    if (!patient.notes.isEmpty())
        html += "<p><b>Napomene:</b> " + escape(patient.notes) + "</p><br/>";

    // Summary stats
    double total = 0;
    for (const auto& iv : interventions) total += iv.cost;
    QString currency = interventions.isEmpty() ? "BAM" : interventions.first().currency;

    if (hidePrice) {
        html += QString("<h2>📊 Sažetak</h2>"
                        "<div class='summary-box'><table>"
                        "<tr><td>Ukupno intervencija:</td><td><b>%1</b></td></tr>"
                        "</table></div>")
                .arg(interventions.size());
    } else {
        html += QString("<h2>📊 Sažetak</h2>"
                        "<div class='summary-box'><table>"
                        "<tr><td>Ukupno intervencija:</td><td><b>%1</b></td></tr>"
                        "<tr><td>Ukupno troškovi:</td><td><b>%2 %3</b></td></tr>"
                        "</table></div>")
                .arg(interventions.size())
                .arg(total, 0, 'f', 2)
                .arg(escape(currency));
    }

    // Interventions
    html += "<h2>🦷 Historija intervencija</h2>";
    if (interventions.isEmpty()) {
        html += "<p style='color:#9E9E9E;'>Nema intervencija.</p>";
    } else {
        for (const auto& iv : interventions) {
            QString dt = iv.dateTime.length() >= 10 ?
                         QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd")
                             .toString("dd.MM.yyyy.") : iv.dateTime;
            if (iv.dateTime.length() >= 16)
                dt += " " + iv.dateTime.right(5);

            QString statusCls   = "status-" + iv.status;
            QString statusLabel = iv.status == "completed" ? "Završeno" :
                                  iv.status == "scheduled" ? "Zakazano" : "Otkazano";

            html += "<div class='iv-card'>";
            html += "<div class='iv-header'>🦷 " + escape(iv.interventionType)
                 + " &nbsp;|&nbsp; " + escape(dt) + "</div>";
            html += "<div class='iv-body'>";

            // Kratka polja — tabela 2 kolone
            auto ivrowShort = [&](const QString& l, const QString& v) {
                if (!v.isEmpty())
                    html += "<div style='display:flex;gap:4px;margin-bottom:2px;font-size:7.5pt;'>"
                            "<span style='font-weight:bold;color:#555;min-width:90px;"
                            "white-space:nowrap;'>" + escape(l) + ":</span>"
                            "<span style='background:#F5F5F5;border-radius:3px;"
                            "padding:1px 6px;flex:1;'>" + escape(v) + "</span>"
                            "</div>";
            };

            // Duga polja — label gore, tekst odmah ispod
            auto ivrowLong = [&](const QString& l, const QString& v) {
                if (!v.isEmpty())
                    html += "<div style='margin-bottom:4px;'>"
                            "<div style='font-weight:bold;color:#00695C;font-size:9pt;"
                            "margin-bottom:2px;'>" + escape(l) + ":</div>"
                            "<div style='background:#F9F9F9;border-left:3px solid #C8E6C9;"
                            "border-radius:0 4px 4px 0;padding:5px 8px;"
                            "white-space:pre-wrap;word-break:break-word;'>"
                            + escape(v) + "</div>"
                            "</div>";
            };

            ivrowShort("Zub",     iv.tooth);
            ivrowShort("Doktor",  iv.doctor);
            ivrowShort("Proizvod",iv.product);

            if (!hidePrice)
                html += QString("<div style='display:flex;gap:4px;margin-bottom:2px;font-size:7.5pt;'>"
                                "<span style='font-weight:bold;color:#555;min-width:90px;'>"
                                "Cijena:</span>"
                                "<span style='background:#FFF9C4;border-radius:3px;"
                                "padding:1px 8px;font-weight:bold;color:#F57F17;'>"
                                "%1 %2</span></div>")
                        .arg(iv.cost, 0, 'f', 2).arg(escape(iv.currency));
            html += QString("<div style='display:flex;gap:6px;margin-bottom:8px;'>"
                            "<span style='font-weight:bold;color:#555;min-width:90px;'>"
                            "Status:</span>"
                            "<span class='%1'>%2</span></div>")
                    .arg(statusCls, statusLabel);

            html += "<hr style='border:none;border-top:1px solid #E0E0E0;margin:3px 0 4px;'/>";

            ivrowLong("Dijagnoza", iv.diagnosis);
            ivrowLong("Terapija",  iv.treatment);
            ivrowLong("Opis rada", iv.description);
            ivrowLong("Napomene",  iv.notes);

            // PDF badge
            if (!iv.pdfPath.isEmpty())
                html += "<div style='margin-bottom:4px;display:inline-block;"
                        "background:#E3F2FD;border-left:3px solid #1565C0;"
                        "padding:3px 8px;border-radius:0 4px 4px 0;font-size:7.5pt;color:#1565C0;'>"
                        "📄 PDF dokument priložen</div><br/>";

            // Potpis intervencije
            html += signatureHtml(iv.signatureImagePath, "✍️ Potpis pacijenta");

            // Fotografije — prikazuju se samo ako je odabrano
            bool hasBefore = !iv.photoBefore.isEmpty() && QFile::exists(iv.photoBefore);
            bool hasAfter  = !iv.photoAfter.isEmpty()  && QFile::exists(iv.photoAfter);
            if (includePhotos && (hasBefore || hasAfter)) {
                html += "<div style='margin-top:6px;page-break-inside:avoid;"
                        "border:1px solid #E0E0E0;border-radius:6px;padding:6px;"
                        "background:#FAFAFA;'>"
                        "<div style='font-weight:bold;color:#1565C0;font-size:7.5pt;"
                        "margin-bottom:4px;'>📷 Fotografije prije/poslije zahvata</div>"
                        "<table width='100%' style='border-collapse:collapse;'><tr>";
                if (hasBefore) {
                    QString imgB = imageToBase64Html(iv.photoBefore, 210, 140);
                    html += "<td style='width:50%;text-align:center;vertical-align:top;"
                            "padding:3px;'>"
                            "<div style='font-size:7pt;font-weight:bold;color:#C62828;"
                            "margin-bottom:2px;background:#FFEBEE;padding:1px 5px;"
                            "border-radius:3px;display:inline-block;'>⬅ PRIJE</div><br/>"
                            + imgB + "</td>";
                } else {
                    html += "<td style='width:50%;'></td>";
                }
                if (hasAfter) {
                    QString imgA = imageToBase64Html(iv.photoAfter, 210, 140);
                    html += "<td style='width:50%;text-align:center;vertical-align:top;"
                            "padding:3px;border-left:1px solid #E0E0E0;'>"
                            "<div style='font-size:7pt;font-weight:bold;color:#2E7D32;"
                            "margin-bottom:2px;background:#E8F5E9;padding:1px 5px;"
                            "border-radius:3px;display:inline-block;'>POSLIJE ➡</div><br/>"
                            + imgA + "</td>";
                } else {
                    html += "<td style='width:50%;'></td>";
                }
                html += "</tr></table></div>";
            }

            // Pristanak badge
            if (!iv.consentPdfPath.isEmpty())
                html += "<div style='margin-bottom:4px;display:inline-block;"
                        "background:#F3E5F5;border-left:3px solid #6A1B9A;"
                        "padding:3px 8px;border-radius:0 4px 4px 0;font-size:7.5pt;color:#6A1B9A;'>"
                        "📝 Pristanak na intervenciju priložen</div><br/>";

            html += signatureHtml(iv.consentSignaturePath,
                                  "✍️ Potpis pristanka na intervenciju");

            html += "</div></div>";

            // Embed PDF stranica ako je odabrano
            if (includePdfs && !iv.pdfPath.isEmpty())
                html += pdfToHtmlImages(iv.pdfPath);
            if (includeHealth && !iv.healthPdfPath.isEmpty())
                html += pdfToHtmlImages(iv.healthPdfPath);
            if (includeConsent && !iv.consentPdfPath.isEmpty())
                html += pdfToHtmlImages(iv.consentPdfPath);
        }
    }

    html += footerHtml();
    html += "</body></html>";
    return html;
}

QString ReportGenerator::buildInterventionHtml(const Intervention& iv,
                                                const Patient& patient)
{
    QString html = "<html><head>" + cssBase() + "</head><body>";
    html += headerHtml();

    html += "<h2>🦷 Izvještaj o intervenciji</h2>";
    html += "<h3>Pacijent: " + escape(patient.fullName()) + "</h3>";

    QString dt = iv.dateTime.length() >= 16 ?
                 QDateTime::fromString(iv.dateTime,"yyyy-MM-dd HH:mm")
                     .toString("dd.MM.yyyy. HH:mm") : iv.dateTime;

    html += "<div class='iv-card'>";
    html += "<div class='iv-header'>🦷 " + escape(iv.interventionType)
         + " — " + escape(dt) + "</div>";
    html += "<div class='iv-body'>";

    // Kratka polja
    auto rowShort = [&](const QString& l, const QString& v) {
        if (!v.isEmpty())
            html += "<div style='display:flex;gap:4px;margin-bottom:2px;font-size:7.5pt;'>"
                    "<span style='font-weight:bold;color:#555;min-width:110px;"
                    "white-space:nowrap;'>" + escape(l) + ":</span>"
                    "<span style='background:#F5F5F5;border-radius:3px;"
                    "padding:1px 6px;flex:1;'>" + escape(v) + "</span>"
                    "</div>";
    };

    // Duga polja — label gore, tekst ispod
    auto rowLong = [&](const QString& l, const QString& v) {
        if (!v.isEmpty())
            html += "<div style='margin-bottom:4px;'>"
                    "<div style='font-weight:bold;color:#00695C;font-size:9pt;"
                    "margin-bottom:2px;'>" + escape(l) + ":</div>"
                    "<div style='background:#F9F9F9;border-left:3px solid #C8E6C9;"
                    "border-radius:0 4px 4px 0;padding:5px 8px;"
                    "white-space:pre-wrap;word-break:break-word;'>"
                    + escape(v) + "</div>"
                    "</div>";
    };

    rowShort("Pacijent",     patient.fullName());
    rowShort("Datum/Vrijeme",dt);
    rowShort("Zub",          iv.tooth);
    rowShort("Doktor",       iv.doctor);
    rowShort("Proizvod",     iv.product);

    html += QString("<div style='display:flex;gap:6px;margin-bottom:8px;'>"
                    "<span style='font-weight:bold;color:#555;min-width:110px;'>"
                    "Cijena:</span>"
                    "<span style='background:#FFF9C4;border-radius:3px;"
                    "padding:1px 8px;font-weight:bold;color:#F57F17;'>"
                    "%1 %2</span></div>")
            .arg(iv.cost, 0, 'f', 2).arg(escape(iv.currency));

    html += "<hr style='border:none;border-top:1px solid #E0E0E0;margin:3px 0 4px;'/>";

    rowLong("Dijagnoza", iv.diagnosis);
    rowLong("Terapija",  iv.treatment);
    rowLong("Opis rada", iv.description);
    rowLong("Napomene",  iv.notes);

    // PDF badge
    if (!iv.pdfPath.isEmpty())
        html += "<div style='margin-bottom:6px;display:inline-block;"
                "background:#E3F2FD;border-left:3px solid #1565C0;"
                "padding:5px 10px;border-radius:0 4px 4px 0;font-size:9pt;color:#1565C0;'>"
                "📄 PDF dokument priložen</div><br/>";

    html += "<hr style='border:none;border-top:1px solid #E0E0E0;margin:8px 0;'/>";

    // Potpis intervencije
    QString sigHtml = signatureHtml(iv.signatureImagePath, "✍️ Potpis pacijenta");
    if (!sigHtml.isEmpty()) {
        html += sigHtml;
    } else {
        html += "<div><b>✍️ Potpis pacijenta:</b>"
                "<div class='sig-line'></div>"
                "<div style='font-size:8pt;color:#757575;'>"
                + escape(patient.fullName()) + "</div></div>";
    }

    // Pristanak badge
    if (!iv.consentPdfPath.isEmpty())
        html += "<div style='margin:6px 0;display:inline-block;"
                "background:#F3E5F5;border-left:3px solid #6A1B9A;"
                "padding:5px 10px;border-radius:0 4px 4px 0;font-size:9pt;color:#6A1B9A;'>"
                "📋 Upitnik/pristanak priložen</div><br/>";

    html += signatureHtml(iv.consentSignaturePath, "✍️ Potpis pristanka na intervenciju");

    // Fotografije
    bool hasBefore = !iv.photoBefore.isEmpty() && QFile::exists(iv.photoBefore);
    bool hasAfter  = !iv.photoAfter.isEmpty()  && QFile::exists(iv.photoAfter);
    if (hasBefore || hasAfter) {
        html += "<div style='margin-top:10px;page-break-inside:avoid;"
                "border:1px solid #E0E0E0;border-radius:6px;padding:8px;background:#FAFAFA;'>"
                "<div style='font-weight:bold;color:#1565C0;font-size:9pt;"
                "margin-bottom:6px;'>📷 Fotografije prije/poslije zahvata</div>"
                "<table width='100%' style='border-collapse:collapse;'><tr>";
        if (hasBefore) {
            QString imgB = imageToBase64Html(iv.photoBefore, 210, 140);
            html += "<td style='width:50%;text-align:center;vertical-align:top;padding:4px;'>"
                    "<div style='font-size:8pt;font-weight:bold;color:#C62828;margin-bottom:3px;"
                    "background:#FFEBEE;padding:2px 6px;border-radius:3px;"
                    "display:inline-block;'>⬅ PRIJE</div><br/>" + imgB + "</td>";
        } else {
            html += "<td style='width:50%;'></td>";
        }
        if (hasAfter) {
            QString imgA = imageToBase64Html(iv.photoAfter, 210, 140);
            html += "<td style='width:50%;text-align:center;vertical-align:top;"
                    "padding:4px;border-left:1px solid #E0E0E0;'>"
                    "<div style='font-size:8pt;font-weight:bold;color:#2E7D32;margin-bottom:3px;"
                    "background:#E8F5E9;padding:2px 6px;border-radius:3px;"
                    "display:inline-block;'>POSLIJE ➡</div><br/>" + imgA + "</td>";
        } else {
            html += "<td style='width:50%;'></td>";
        }
        html += "</tr></table></div>";
    }

    html += "</div></div>";
    html += footerHtml();
    html += "</body></html>";
    return html;
}

void ReportGenerator::printPatientReport(const Patient& patient,
                                          const QList<Intervention>& interventions,
                                          QWidget* parent,
                                          bool includePdfs,
                                          bool includeConsent,
                                          bool includeHealth,
                                          bool includePhotos,
                                          bool hidePrice)
{
    QString html = buildPatientReportHtml(patient, interventions,
                                          includePdfs, includeConsent,
                                          includeHealth, includePhotos, hidePrice);
    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(8,8,8,8), QPageLayout::Millimeter);

    QPrintPreviewDialog preview(&printer, parent);
    auto& cs = ClinicSettings::instance();
    preview.setWindowTitle("Pregled ispisa — " + patient.fullName() +
                           " | " + cs.clinicName);
    preview.resize(900, 700);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [&](QPrinter* p) { doc.print(p); });
    preview.exec();
}

void ReportGenerator::printIntervention(const Intervention& iv,
                                         const Patient& patient,
                                         QWidget* parent)
{
    QString html = buildInterventionHtml(iv, patient);
    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(8,8,8,8), QPageLayout::Millimeter);

    QPrintPreviewDialog preview(&printer, parent);
    auto& cs = ClinicSettings::instance();
    preview.setWindowTitle("Ispis intervencije — " + cs.clinicName);
    preview.resize(900, 700);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [&](QPrinter* p) { doc.print(p); });
    preview.exec();
}
