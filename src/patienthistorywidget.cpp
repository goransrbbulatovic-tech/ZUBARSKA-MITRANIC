#include "patienthistorywidget.h"
#include "database.h"
#include <QCalendarWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QTextCharFormat>
#include <QFont>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

// ── Custom calendar to highlight treatment days ───────────────────────────────
class DentalCalendar : public QCalendarWidget
{
public:
    explicit DentalCalendar(QWidget* parent = nullptr) : QCalendarWidget(parent) {
        setGridVisible(true);
        setNavigationBarVisible(true);
        setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        setStyleSheet(R"(
            QCalendarWidget QAbstractItemView {
                font-size: 11px;
                selection-background-color: #1565C0;
                selection-color: white;
            }
            QCalendarWidget QWidget#qt_calendar_navigationbar {
                background: #1565C0;
                padding: 4px;
            }
            QCalendarWidget QToolButton {
                color: white; font-weight: bold; font-size: 13px;
                background: transparent; border: none;
            }
            QCalendarWidget QToolButton:hover { background: rgba(255,255,255,0.2); border-radius:3px; }
            QCalendarWidget QSpinBox { color: white; background: transparent; border:none; font-weight:bold; }
            QCalendarWidget QMenu { color: #212121; background: white; }
        )");
    }

    void setMarkedDates(const QList<QDate>& dates)
    {
        // Clear previous marks
        QTextCharFormat normal;
        setDateTextFormat(QDate(), normal);

        QTextCharFormat marked;
        marked.setBackground(QColor("#C8E6C9"));
        marked.setForeground(QColor("#1B5E20"));
        QFont f = marked.font();
        f.setBold(true);
        marked.setFont(f);

        for (const QDate& d : dates)
            setDateTextFormat(d, marked);
    }
};

// ─────────────────────────────────────────────────────────────────────────────

PatientHistoryWidget::PatientHistoryWidget(QWidget* parent) : QWidget(parent)
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0,0,0,0);
    mainLay->setSpacing(0);

    // ── Splitter: left = calendar, right = detail ─────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet("QSplitter::handle{background:#E0E0E0;}");

    // Left pane
    auto* leftWidget = new QWidget;
    leftWidget->setStyleSheet("background:#FAFAFA;");
    auto* leftLay = new QVBoxLayout(leftWidget);
    leftLay->setContentsMargins(12,12,12,12);
    leftLay->setSpacing(8);

    auto* calTitle = new QLabel("📅  Kalendar intervencija");
    calTitle->setStyleSheet("font-size:14px; font-weight:bold; color:#1565C0;");
    leftLay->addWidget(calTitle);

    auto* legend = new QLabel("🟩 Dan s intervencijom   🔵 Odabrani dan");
    legend->setStyleSheet("font-size:10px; color:#757575;");
    leftLay->addWidget(legend);

    m_calendar = new DentalCalendar;
    m_calendar->setMinimumWidth(280);
    leftLay->addWidget(m_calendar);

    m_dateLbl = new QLabel("Odaberite datum");
    m_dateLbl->setStyleSheet("font-size:13px; font-weight:bold; color:#424242; padding:4px 0;");
    leftLay->addWidget(m_dateLbl);

    m_ivList = new QListWidget;
    m_ivList->setStyleSheet(R"(
        QListWidget { border:1px solid #E0E0E0; border-radius:4px; background:white; }
        QListWidget::item { padding:8px 6px; border-bottom:1px solid #F5F5F5; }
        QListWidget::item:selected { background:#E3F2FD; color:#1565C0; }
        QListWidget::item:hover { background:#F5F5F5; }
    )");
    m_ivList->setAlternatingRowColors(false);
    leftLay->addWidget(m_ivList, 1);

    // Action buttons
    auto* actRow = new QHBoxLayout;
    auto* editBtn      = new QPushButton("✏️ Uredi");
    auto* delBtn       = new QPushButton("🗑 Briši");
    auto* pdfBtn       = new QPushButton("📄 Otvori PDF");
    auto* consentBtn   = new QPushButton("📋 Otvori upitnik");

    editBtn->setObjectName("editBtn");
    delBtn->setObjectName("delBtn");
    pdfBtn->setObjectName("pdfBtn");
    consentBtn->setObjectName("consentBtn");

    auto btnStyle = [](const QString& bg, const QString& hov) {
        return QString("QPushButton{background:%1;color:white;border-radius:4px;"
                       "border:none;padding:5px 8px;font-size:11px;font-weight:bold;}"
                       "QPushButton:hover{background:%2;}").arg(bg,hov);
    };
    editBtn->setStyleSheet(btnStyle("#1565C0","#1976D2"));
    delBtn->setStyleSheet(btnStyle("#EF5350","#E53935"));
    pdfBtn->setStyleSheet(btnStyle("#00695C","#00897B"));
    consentBtn->setStyleSheet(btnStyle("#6A1B9A","#7B1FA2"));

    actRow->addWidget(editBtn);
    actRow->addWidget(delBtn);
    actRow->addWidget(pdfBtn);
    actRow->addWidget(consentBtn);
    leftLay->addLayout(actRow);

    splitter->addWidget(leftWidget);

    // Right pane
    auto* rightWidget = new QWidget;
    rightWidget->setStyleSheet("background:white;");
    auto* rightLay = new QVBoxLayout(rightWidget);
    rightLay->setContentsMargins(12,12,12,12);

    auto* detailTitle = new QLabel("📋  Detalji intervencije");
    detailTitle->setStyleSheet("font-size:14px; font-weight:bold; color:#00695C;");
    rightLay->addWidget(detailTitle);

    m_detail = new QTextBrowser;
    m_detail->setStyleSheet("border:1px solid #E0E0E0; border-radius:4px; background:white;");
    m_detail->setOpenExternalLinks(false);
    rightLay->addWidget(m_detail);

    splitter->addWidget(rightWidget);
    splitter->setSizes({320, 500});

    mainLay->addWidget(splitter);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_calendar, &QCalendarWidget::clicked, this, &PatientHistoryWidget::onDateSelected);
    connect(m_ivList, &QListWidget::currentRowChanged, this, &PatientHistoryWidget::onInterventionClicked);
    connect(editBtn,    &QPushButton::clicked, this, &PatientHistoryWidget::onEditClicked);
    connect(delBtn,     &QPushButton::clicked, this, &PatientHistoryWidget::onDeleteClicked);
    connect(pdfBtn,     &QPushButton::clicked, this, &PatientHistoryWidget::onViewPdfClicked);
    connect(consentBtn, &QPushButton::clicked, this, &PatientHistoryWidget::onViewConsentClicked);

    m_detail->setHtml("<p style='color:#9E9E9E; text-align:center; margin-top:40px;'>"
                      "Kliknite na datum u kalendaru da vidite intervencije</p>");
}

void PatientHistoryWidget::setPatient(const Patient& patient)
{
    m_patient = patient;
    refresh();
}

void PatientHistoryWidget::refresh()
{
    loadInterventionDates();
    onDateSelected(m_calendar->selectedDate());
}

void PatientHistoryWidget::loadInterventionDates()
{
    m_markedDates = Database::instance().getInterventionDates(m_patient.id);
    static_cast<DentalCalendar*>(m_calendar)->setMarkedDates(m_markedDates);
}

void PatientHistoryWidget::onDateSelected(const QDate& date)
{
    m_dateLbl->setText("📅 " + date.toString("dd. MMMM yyyy."));
    loadInterventionsForDate(date);
}

void PatientHistoryWidget::loadInterventionsForDate(const QDate& date)
{
    m_currentIvs = Database::instance().getInterventionsByDate(m_patient.id, date);
    m_ivList->clear();

    if (m_currentIvs.isEmpty()) {
        m_ivList->addItem("  Nema intervencija za ovaj dan");
        m_detail->setHtml("<p style='color:#9E9E9E; text-align:center; margin-top:40px;'>"
                          "Nema intervencija za odabrani datum</p>");
        return;
    }

    for (const auto& iv : m_currentIvs) {
        QString time = iv.dateTime.length() >= 16 ? iv.dateTime.right(5) : "";
        QString icon = iv.status == "completed" ? "✅" :
                       iv.status == "scheduled" ? "📅" : "❌";
        m_ivList->addItem(QString("%1 %2  –  %3").arg(icon, time, iv.interventionType));
    }
    if (!m_currentIvs.isEmpty()) {
        m_ivList->setCurrentRow(0);
        onInterventionClicked(0);
    }
}

void PatientHistoryWidget::onInterventionClicked(int row)
{
    if (row < 0 || row >= m_currentIvs.size()) return;
    m_detail->setHtml(buildInterventionHtml(m_currentIvs[row]));
}

QString PatientHistoryWidget::buildInterventionHtml(const Intervention& iv)
{
    QString statusColor = iv.status=="completed" ? "#2E7D32" :
                          iv.status=="scheduled" ? "#1565C0" : "#C62828";
    QString statusLabel = iv.status=="completed" ? "Završeno" :
                          iv.status=="scheduled" ? "Zakazano" : "Otkazano";

    QString html = R"(
    <html><body style='font-family:Segoe UI,Arial; margin:8px; color:#212121;
    overflow-x:hidden; word-wrap:break-word;'>
    <h2 style='color:#00695C; border-bottom:2px solid #C8E6C9; padding-bottom:6px;
    font-size:13pt; margin-bottom:10px;'>
    )";
    html += "🦷 " + iv.interventionType.toHtmlEscaped();
    html += "</h2>";

    // Helper — kratka polja (2 kolone)
    auto rowShort = [&](const QString& lbl, const QString& val) {
        if (val.isEmpty()) return;
        html += "<div style='display:flex;gap:6px;margin-bottom:4px;'>"
                "<span style='font-weight:bold;color:#555;white-space:nowrap;min-width:120px;'>"
                + lbl + "</span>"
                "<span style='background:#F5F5F5;border-radius:3px;padding:2px 6px;"
                "flex:1;'>" + val.toHtmlEscaped() + "</span>"
                "</div>";
    };

    // Helper — duga polja (label gore, tekst ispod)
    auto rowLong = [&](const QString& lbl, const QString& val) {
        if (val.isEmpty()) return;
        html += "<div style='margin-bottom:8px;'>"
                "<div style='font-weight:bold;color:#00695C;font-size:9pt;"
                "margin-bottom:2px;'>" + lbl + "</div>"
                "<div style='background:#F9F9F9;border-left:3px solid #C8E6C9;"
                "border-radius:0 4px 4px 0;padding:5px 8px;"
                "white-space:pre-wrap;word-break:break-word;'>"
                + val.toHtmlEscaped() + "</div>"
                "</div>";
    };

    QString dt = iv.dateTime;
    if (dt.length() >= 16)
        dt = QDateTime::fromString(iv.dateTime,"yyyy-MM-dd HH:mm").toString("dd.MM.yyyy  HH:mm");

    // Kratka polja — u dvijema kolonama
    rowShort("📅 Datum:", dt);
    rowShort("👨‍⚕️ Doktor:", iv.doctor);
    rowShort("🦷 Zub:", iv.tooth);
    if (!iv.product.isEmpty())
        rowShort("💊 Proizvod:", iv.product);

    // Cijena i status
    QString cijenaStr = QString("%1 %2").arg(iv.cost, 0, 'f', 2).arg(iv.currency);
    rowShort("💰 Cijena:", cijenaStr);
    rowShort("Status:", statusLabel);

    html += "<hr style='border:none;border-top:1px solid #E0E0E0;margin:8px 0;'/>";

    // Duga polja — label gore, tekst odmah ispod
    rowLong("📋 Dijagnoza:", iv.diagnosis);
    rowLong("💊 Terapija:", iv.treatment);
    rowLong("📝 Opis rada:", iv.description);
    rowLong("🗒 Napomene:", iv.notes);

    // PDF badge
    if (!iv.pdfPath.isEmpty()) {
        html += "<div style='margin-bottom:6px;'>"
                "<span style='background:#E3F2FD;border-left:3px solid #1565C0;"
                "padding:4px 10px;border-radius:0 4px 4px 0;color:#1565C0;font-size:9pt;'>"
                "📄 PDF dokument priložen</span></div>";
    }
    if (!iv.signatureImagePath.isEmpty()) {
        html += "<div style='margin-bottom:6px;'>"
                "<div style='font-weight:bold;color:#555;margin-bottom:2px;'>✍️ Potpis pacijenta:</div>"
                "<img src='" + iv.signatureImagePath + "' style='border:1px solid #E0E0E0;"
                "border-radius:4px;max-width:230px;max-height:80px;'/>"
                "</div>";
    }

    // Fotografije — jedna do druge, bez klizaca
    bool hasBefore = !iv.photoBefore.isEmpty() && QFile::exists(iv.photoBefore);
    bool hasAfter  = !iv.photoAfter.isEmpty()  && QFile::exists(iv.photoAfter);
    if (hasBefore || hasAfter) {
        html += "<div style='border:1px solid #E0E0E0;border-radius:6px;"
                "padding:8px;background:#FAFAFA;margin-bottom:6px;'>"
                "<div style='font-weight:bold;color:#1565C0;font-size:9pt;"
                "margin-bottom:5px;'>📷 Fotografije prije/poslije zahvata</div>"
                "<table width='100%' style='border-collapse:collapse;table-layout:fixed;'><tr>";
        if (hasBefore) {
            html += "<td style='width:50%;text-align:center;vertical-align:top;padding:3px;'>"
                    "<div style='font-size:8pt;font-weight:bold;color:#C62828;background:#FFEBEE;"
                    "padding:1px 6px;border-radius:3px;display:inline-block;"
                    "margin-bottom:3px;'>⬅ PRIJE</div><br/>"
                    "<img src='" + iv.photoBefore + "' style='max-width:100%;max-height:145px;"
                    "border:2px solid #C62828;border-radius:5px;'/></td>";
        } else {
            html += "<td style='width:50%;'></td>";
        }
        if (hasAfter) {
            html += "<td style='width:50%;text-align:center;vertical-align:top;"
                    "padding:3px;border-left:1px solid #E0E0E0;'>"
                    "<div style='font-size:8pt;font-weight:bold;color:#2E7D32;background:#E8F5E9;"
                    "padding:1px 6px;border-radius:3px;display:inline-block;"
                    "margin-bottom:3px;'>POSLIJE ➡</div><br/>"
                    "<img src='" + iv.photoAfter + "' style='max-width:100%;max-height:145px;"
                    "border:2px solid #2E7D32;border-radius:5px;'/></td>";
        } else {
            html += "<td style='width:50%;'></td>";
        }
        html += "</tr></table></div>";
    }

    // Pristanak potpis
    if (!iv.consentSignaturePath.isEmpty() && QFile::exists(iv.consentSignaturePath)) {
        html += "<div style='margin-bottom:6px;'>"
                "<div style='font-weight:bold;color:#555;margin-bottom:2px;'>📝 Potpis pristanka:</div>"
                "<img src='" + iv.consentSignaturePath + "' style='border:1px solid #CE93D8;"
                "border-radius:4px;max-width:230px;max-height:80px;'/>"
                "</div>";
    }
    if (!iv.consentPdfPath.isEmpty()) {
        html += "<div style='margin-bottom:6px;'>"
                "<span style='background:#F3E5F5;border-left:3px solid #6A1B9A;"
                "padding:4px 10px;border-radius:0 4px 4px 0;color:#6A1B9A;font-size:9pt;'>"
                "📋 Upitnik/pristanak priložen</span></div>";
    }

    html += "</body></html>";
    return html;
}

void PatientHistoryWidget::onEditClicked()
{
    int row = m_ivList->currentRow();
    if (row < 0 || row >= m_currentIvs.size()) return;
    emit editIntervention(m_currentIvs[row].id);
}

void PatientHistoryWidget::onDeleteClicked()
{
    int row = m_ivList->currentRow();
    if (row < 0 || row >= m_currentIvs.size()) return;

    auto reply = QMessageBox::question(this, "Brisanje",
        "Da li ste sigurni da želite obrisati ovu intervenciju?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        emit deleteIntervention(m_currentIvs[row].id);
    }
}

void PatientHistoryWidget::onViewPdfClicked()
{
    int row = m_ivList->currentRow();
    if (row < 0 || row >= m_currentIvs.size()) return;
    const QString& path = m_currentIvs[row].pdfPath;
    if (path.isEmpty()) {
        QMessageBox::information(this, "PDF", "Ova intervencija nema priložen PDF dokument.");
        return;
    }
    emit viewPdf(path);
}

void PatientHistoryWidget::onViewConsentClicked()
{
    int row = m_ivList->currentRow();
    if (row < 0 || row >= m_currentIvs.size()) return;
    const QString& path = m_currentIvs[row].consentPdfPath;
    if (path.isEmpty()) {
        QMessageBox::information(this, "Upitnik",
            "Ova intervencija nema priložen PDF upitnika/pristanka.");
        return;
    }
    emit viewPdf(path);
}
