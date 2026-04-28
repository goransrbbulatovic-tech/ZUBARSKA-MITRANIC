#include "interventiondialog.h"
#include "signaturewidget.h"
#include "clinicsettings.h"
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QPixmap>
#include <QFile>
#include <QFrame>
#include <QTabWidget>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>

InterventionDialog::InterventionDialog(QWidget* parent, int patientId, const Intervention& iv)
    : QDialog(parent), m_iv(iv), m_patientId(patientId)
{
    setWindowTitle(iv.id == 0 ? "Nova Intervencija" : "Uredi Intervenciju");
    setMinimumWidth(660);
    setMinimumHeight(580);
    setModal(true);
    setupUi();
    if (iv.id != 0) populateFields();
}

void InterventionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    // ── Header ────────────────────────────────────────────────────────────────
    auto* header = new QWidget;
    header->setStyleSheet("background:#00695C; padding:16px;");
    auto* hLay = new QHBoxLayout(header);
    auto* title = new QLabel(m_iv.id == 0 ? "🦷  Nova Intervencija" : "🦷  Uredi Intervenciju");
    title->setStyleSheet("color:white; font-size:18px; font-weight:bold;");
    hLay->addWidget(title);
    mainLayout->addWidget(header);

    auto fieldStyle = QString("QLineEdit,QDateTimeEdit,QComboBox,QDoubleSpinBox{"
                              "border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;"
                              "min-height:32px;}"
                              "QLineEdit:focus,QDateTimeEdit:focus,QComboBox:focus,QDoubleSpinBox:focus{"
                              "border:2px solid #00695C;}");
    auto textStyle = QString("QTextEdit{border:1px solid #BDBDBD;border-radius:4px;padding:4px;}"
                             "QTextEdit:focus{border:2px solid #00695C;}");
    auto lblStyle  = QString("font-weight:600; color:#424242;");

    auto makeLbl = [&](const QString& t){
        auto* l = new QLabel(t); l->setStyleSheet(lblStyle); return l;
    };
    auto makeGroup = [&](const QString& t) {
        auto* g = new QGroupBox(t);
        g->setStyleSheet("QGroupBox{font-weight:bold;color:#1B5E20;"
                         "border:1px solid #C8E6C9;border-radius:6px;margin-top:8px;"
                         "padding-top:8px;}"
                         "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px;}");
        return g;
    };

    // ── TABS ──────────────────────────────────────────────────────────────────
    auto* tabs = new QTabWidget;
    tabs->setStyleSheet(
        "QTabWidget::pane{border:none;background:#FAFAFA;}"
        "QTabBar::tab{background:#E8F5E9;color:#1B5E20;padding:8px 16px;"
        "border:1px solid #C8E6C9;border-bottom:none;border-radius:4px 4px 0 0;"
        "font-weight:bold;margin-right:2px;}"
        "QTabBar::tab:selected{background:#00695C;color:white;}"
        "QTabBar::tab:hover:!selected{background:#C8E6C9;}");

    // ═══════════════════════════════════════════════════════════════
    // TAB 1 — Osnovno
    // ═══════════════════════════════════════════════════════════════
    auto* tab1 = new QWidget;
    tab1->setStyleSheet("background:#FAFAFA;");
    auto* t1lay = new QVBoxLayout(tab1);
    t1lay->setContentsMargins(16,16,16,16);
    t1lay->setSpacing(12);

    auto* basicGroup = makeGroup("Osnovno");
    auto* basicForm  = new QFormLayout(basicGroup);
    basicForm->setSpacing(8);

    // Vrsta intervencije — sa dugmetom za dodavanje novog tipa
    auto& cs = ClinicSettings::instance();
    m_type = new QComboBox;
    m_type->setEditable(true);

    // Standardne intervencije
    QStringList defaultTypes = {
        "Pregled", "Hitna pomoć", "Konsultacija",
        // Restorativne
        "Plomba (kompozit)", "Plomba (amalgam)", "Privremena plomba",
        "Devitalizacija", "Endodoncija (liječenje kanala)",
        // Hirurške
        "Vađenje zuba", "Hirurško vađenje", "Operacija ciste",
        "Ugradnja implanta", "Augmentacija kosti",
        // Protetika
        "Krunica (keramička)", "Krunica (metalno-keramička)", "Krunica (cirkon)",
        "Most", "Djelimična proteza", "Totalna proteza", "Skeletirana proteza",
        // Estetske
        "Beljenje zuba (ordinacija)", "Beljenje zuba (kućno)",
        "Keramičke ljuske (Veneers)", "Kompozitne ljuske",
        "Estetska restauracija", "Kompozitna nadogradnja zuba",
        "Estetsko preoblikovanje osmijeha", "Gingivoplastika",
        "Korekcija desni laserom",
        // Ortodoncija
        "Ortodoncija — pregled", "Fiksni aparat", "Mobilni aparat",
        "Retainer", "Invisalign/Providni aligner",
        // Parodontologija
        "Čišćenje kamenca", "Pjeskarenje", "Poliranje",
        "Kiretaža džepa", "Gingivektomija", "Parodontalni pregled",
        // Ostalo
        "Rendgen (periapikalni)", "Rendgen (panoramski)", "CBCT snimak",
        "Fluoridacija", "Pečaćenje fisura", "Izrada udlage"
    };

    // Dodaj custom tipove koje je korisnik sačuvao
    for (const QString& t : cs.customInterventionTypes)
        if (!defaultTypes.contains(t)) defaultTypes.append(t);

    m_type->addItems(defaultTypes);
    m_type->setStyleSheet(fieldStyle);
    connect(m_type, &QComboBox::currentTextChanged,
            this, &InterventionDialog::onTypeChanged);

    // Dugme za dodavanje novog tipa
    auto* addTypeBtn = new QPushButton("+ Novi tip");
    addTypeBtn->setStyleSheet("QPushButton{background:#1565C0;color:white;border:none;"
                               "border-radius:4px;padding:6px 10px;font-size:10px;font-weight:bold;}"
                               "QPushButton:hover{background:#1976D2;}");
    addTypeBtn->setFixedHeight(34);

    auto* typeRow = new QHBoxLayout;
    typeRow->addWidget(m_type, 1);
    typeRow->addWidget(addTypeBtn);
    basicForm->addRow(makeLbl("Vrsta intervencije*:"), typeRow);

    connect(addTypeBtn, &QPushButton::clicked, [this](){
        bool ok;
        QString novi = QInputDialog::getText(this, "Novi tip intervencije",
            "Unesite naziv nove vrste intervencije:",
            QLineEdit::Normal, "", &ok);
        if (!ok || novi.trimmed().isEmpty()) return;
        novi = novi.trimmed();
        if (m_type->findText(novi) < 0) {
            m_type->addItem(novi);
            // Sačuvaj u settings
            auto& cs2 = ClinicSettings::instance();
            if (!cs2.customInterventionTypes.contains(novi)) {
                cs2.customInterventionTypes.append(novi);
                cs2.save();
            }
        }
        m_type->setCurrentText(novi);
    });

    // Zub
    m_tooth = new QLineEdit;
    m_tooth->setPlaceholderText("npr. 16, 21, 36...");
    m_tooth->setStyleSheet(fieldStyle);
    basicForm->addRow(makeLbl("Zub:"), m_tooth);

    // Proizvod/materijal
    m_product = new QLineEdit;
    m_product->setPlaceholderText("npr. 3M Filtek Z550, Ivoclar IPS e.max...");
    m_product->setStyleSheet(fieldStyle);
    basicForm->addRow(makeLbl("Proizvod/materijal:"), m_product);

    // Doktor
    m_doctorCombo = new QComboBox;
    m_doctorCombo->setEditable(true);
    if (cs.doctors.isEmpty())
        m_doctorCombo->addItem("Dr.");
    else
        m_doctorCombo->addItems(cs.doctors);
    m_doctorCombo->setStyleSheet(fieldStyle);
    basicForm->addRow(makeLbl("Doktor:"), m_doctorCombo);

    // Datum
    m_dateTime = new QDateTimeEdit(QDateTime::currentDateTime());
    m_dateTime->setCalendarPopup(true);
    m_dateTime->setDisplayFormat("dd.MM.yyyy HH:mm");
    m_dateTime->setStyleSheet(fieldStyle);
    basicForm->addRow(makeLbl("Datum i vrijeme:"), m_dateTime);

    // Status
    m_status = new QComboBox;
    m_status->addItems({"completed","scheduled","cancelled"});
    m_status->setStyleSheet(fieldStyle);
    basicForm->addRow(makeLbl("Status:"), m_status);

    t1lay->addWidget(basicGroup);
    t1lay->addStretch();
    tabs->addTab(tab1, "📋 Osnovno");

    // ═══════════════════════════════════════════════════════════════
    // TAB 2 — Medicinski podaci
    // ═══════════════════════════════════════════════════════════════
    auto* tab2scroll = new QScrollArea;
    tab2scroll->setFrameShape(QFrame::NoFrame);
    tab2scroll->setWidgetResizable(true);
    auto* tab2 = new QWidget;
    tab2->setStyleSheet("background:#FAFAFA;");
    auto* t2lay = new QVBoxLayout(tab2);
    t2lay->setContentsMargins(16,16,16,16);
    t2lay->setSpacing(12);

    auto* medGroup = makeGroup("Medicinski podaci");
    auto* medForm  = new QFormLayout(medGroup);
    medForm->setSpacing(8);

    m_diagnosis = new QTextEdit;
    m_diagnosis->setPlaceholderText("Dijagnoza...");
    m_diagnosis->setFixedHeight(80);
    m_diagnosis->setStyleSheet(textStyle);
    medForm->addRow(makeLbl("Dijagnoza:"), m_diagnosis);

    m_treatment = new QTextEdit;
    m_treatment->setPlaceholderText("Terapija...");
    m_treatment->setFixedHeight(80);
    m_treatment->setStyleSheet(textStyle);
    medForm->addRow(makeLbl("Terapija:"), m_treatment);

    m_description = new QTextEdit;
    m_description->setPlaceholderText("Detaljan opis rada...");
    m_description->setFixedHeight(80);
    m_description->setStyleSheet(textStyle);
    medForm->addRow(makeLbl("Opis rada:"), m_description);

    m_notes = new QTextEdit;
    m_notes->setPlaceholderText("Napomene za narednu posjetu, alergije, specifičnosti...");
    m_notes->setFixedHeight(80);
    m_notes->setStyleSheet(textStyle);
    medForm->addRow(makeLbl("Napomene:"), m_notes);

    t2lay->addWidget(medGroup);
    t2lay->addStretch();
    tab2scroll->setWidget(tab2);
    tabs->addTab(tab2scroll, "🩺 Medicinski podaci");

    // ═══════════════════════════════════════════════════════════════
    // TAB 3 — Finansije, PDF i Potpis
    // ═══════════════════════════════════════════════════════════════
    auto* tab3scroll = new QScrollArea;
    tab3scroll->setFrameShape(QFrame::NoFrame);
    tab3scroll->setWidgetResizable(true);
    auto* tab3 = new QWidget;
    tab3->setStyleSheet("background:#FAFAFA;");
    auto* t3lay = new QVBoxLayout(tab3);
    t3lay->setContentsMargins(16,16,16,16);
    t3lay->setSpacing(12);

    // Finansije
    auto* finGroup = makeGroup("Finansije");
    auto* finForm  = new QFormLayout(finGroup);
    finForm->setSpacing(8);
    m_cost = new QDoubleSpinBox;
    m_cost->setRange(0, 99999);
    m_cost->setDecimals(2);
    m_cost->setSingleStep(5.0);
    m_cost->setStyleSheet(fieldStyle);
    m_currency = new QComboBox;
    m_currency->addItems({"BAM","EUR","USD","RSD"});
    m_currency->setStyleSheet(fieldStyle);
    auto* costRow = new QHBoxLayout;
    costRow->addWidget(m_cost, 2);
    costRow->addWidget(m_currency, 1);
    finForm->addRow(makeLbl("Cijena:"), costRow);
    t3lay->addWidget(finGroup);

    // PDF
    auto* docGroup = makeGroup("Priloženi dokument (PDF)");
    auto* docLay   = new QVBoxLayout(docGroup);
    auto* pdfRow   = new QHBoxLayout;
    m_pdfPath = new QLineEdit;
    m_pdfPath->setPlaceholderText("Putanja do PDF dokumenta...");
    m_pdfPath->setReadOnly(true);
    m_pdfPath->setStyleSheet(fieldStyle);
    auto* browseBtn = new QPushButton("📎 Priloži PDF");
    browseBtn->setStyleSheet("QPushButton{background:#1565C0;color:white;border-radius:4px;"
                             "border:none;padding:6px 12px;font-weight:bold;}"
                             "QPushButton:hover{background:#1976D2;}");
    pdfRow->addWidget(m_pdfPath);
    pdfRow->addWidget(browseBtn);
    docLay->addLayout(pdfRow);
    t3lay->addWidget(docGroup);

    // Potpis
    auto* sigGroup = makeGroup("Elektronski potpis pacijenta");
    auto* sigLay   = new QVBoxLayout(sigGroup);
    m_signature = new SignatureWidget;
    sigLay->addWidget(m_signature);
    auto* clearSigBtn = new QPushButton("🗑 Obriši potpis");
    clearSigBtn->setStyleSheet("QPushButton{background:#EF5350;color:white;border-radius:4px;"
                               "border:none;padding:4px 10px;}"
                               "QPushButton:hover{background:#E53935;}");
    auto* sigBtnRow = new QHBoxLayout;
    sigBtnRow->addStretch();
    sigBtnRow->addWidget(clearSigBtn);
    sigLay->addLayout(sigBtnRow);
    t3lay->addWidget(sigGroup);
    t3lay->addStretch();

    connect(browseBtn,   &QPushButton::clicked, this, &InterventionDialog::browsePdf);
    connect(clearSigBtn, &QPushButton::clicked, this, &InterventionDialog::clearSignature);

    tab3scroll->setWidget(tab3);
    tabs->addTab(tab3scroll, "💰 Finansije & Potpis");

    // ═══════════════════════════════════════════════════════════════
    // TAB 4 — Fotografije
    // ═══════════════════════════════════════════════════════════════
    auto* tab4 = new QWidget;
    tab4->setStyleSheet("background:#FAFAFA;");
    auto* t4lay = new QVBoxLayout(tab4);
    t4lay->setContentsMargins(16,16,16,16);

    auto* photoGroup = makeGroup("Fotografije zuba (prije/poslije)");
    auto* photoLay   = new QHBoxLayout(photoGroup);
    photoLay->setSpacing(12);

    auto makePhotoBox = [&](const QString& label, QLabel*& preview,
                             const char* slot) -> QWidget* {
        auto* box = new QWidget;
        box->setStyleSheet("background:white;border:1px solid #E0E0E0;border-radius:6px;");
        auto* bl = new QVBoxLayout(box);
        bl->setContentsMargins(8, 8, 8, 8);
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("font-weight:bold;color:#424242;font-size:10px;");
        preview = new QLabel;
        preview->setFixedSize(220, 150);
        preview->setAlignment(Qt::AlignCenter);
        preview->setStyleSheet("border:1px dashed #BDBDBD;border-radius:4px;"
                               "color:#9E9E9E;font-size:10px;");
        preview->setText("Nema fotografije");
        auto* btn = new QPushButton("📷 Učitaj fotografiju");
        btn->setStyleSheet("QPushButton{background:#1565C0;color:white;border-radius:4px;"
                           "border:none;padding:5px;font-size:10px;}"
                           "QPushButton:hover{background:#1976D2;}");
        connect(btn, SIGNAL(clicked()), this, slot);
        bl->addWidget(lbl);
        bl->addWidget(preview);
        bl->addWidget(btn);
        return box;
    };

    photoLay->addWidget(makePhotoBox("📷  PRIJE zahvata",
        m_photoBeforePrev, SLOT(browsePhotoBefore())));
    photoLay->addWidget(makePhotoBox("📷  POSLIJE zahvata",
        m_photoAfterPrev,  SLOT(browsePhotoAfter())));
    t4lay->addWidget(photoGroup);
    t4lay->addStretch();
    tabs->addTab(tab4, "📷 Fotografije");

    // ═══════════════════════════════════════════════════════════════
    // TAB 5 — Upitnik / Pristanak
    // ═══════════════════════════════════════════════════════════════
    // TAB 5 — Upitnik o zdravstvenom stanju
    // ═══════════════════════════════════════════════════════════════
    auto* tab5scroll = new QScrollArea;
    tab5scroll->setFrameShape(QFrame::NoFrame);
    tab5scroll->setWidgetResizable(true);
    auto* tab5 = new QWidget;
    tab5->setStyleSheet("background:#FAFAFA;");
    auto* t5lay = new QVBoxLayout(tab5);
    t5lay->setContentsMargins(16,16,16,16);
    t5lay->setSpacing(12);

    auto* healthInfo = new QLabel(
        "<b>Upitnik o zdravstvenom stanju</b><br/>"
        "<span style='color:#555;font-size:11px;'>"
        "Priložite popunjen upitnik o zdravstvenom stanju pacijenta (PDF)."
        "</span>");
    healthInfo->setWordWrap(true);
    healthInfo->setStyleSheet("background:#E3F2FD;border:1px solid #BBDEFB;"
                              "border-radius:6px;padding:10px;");
    t5lay->addWidget(healthInfo);

    auto* healthGroup = makeGroup("Upitnik o zdravstvenom stanju (PDF)");
    auto* hLay = new QVBoxLayout(healthGroup);
    auto* hPdfRow = new QHBoxLayout;
    m_healthPdfPath = new QLineEdit;
    m_healthPdfPath->setPlaceholderText("Priložite PDF upitnika o zdravstvenom stanju...");
    m_healthPdfPath->setReadOnly(true);
    m_healthPdfPath->setStyleSheet(fieldStyle);
    auto* browseHealthBtn = new QPushButton("📎 Priloži PDF");
    browseHealthBtn->setStyleSheet(
        "QPushButton{background:#1565C0;color:white;border-radius:4px;"
        "border:none;padding:6px 12px;font-weight:bold;}"
        "QPushButton:hover{background:#1976D2;}");
    auto* openHealthBtn = new QPushButton("🔍 Otvori");
    openHealthBtn->setStyleSheet(
        "QPushButton{background:#00695C;color:white;border-radius:4px;"
        "border:none;padding:6px 10px;font-weight:bold;}"
        "QPushButton:hover{background:#00897B;}");
    hPdfRow->addWidget(m_healthPdfPath, 1);
    hPdfRow->addWidget(browseHealthBtn);
    hPdfRow->addWidget(openHealthBtn);
    hLay->addLayout(hPdfRow);
    t5lay->addWidget(healthGroup);
    t5lay->addStretch();

    connect(browseHealthBtn, &QPushButton::clicked,
            this, &InterventionDialog::browseHealthPdf);
    connect(openHealthBtn, &QPushButton::clicked, [this](){
        if (!m_healthPdfPath->text().isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_healthPdfPath->text()));
    });

    tab5scroll->setWidget(tab5);
    tabs->addTab(tab5scroll, "🏥 Upitnik o zdravlju");

    // ═══════════════════════════════════════════════════════════════
    // TAB 6 — Pristanak na intervenciju
    // ═══════════════════════════════════════════════════════════════
    auto* tab6scroll = new QScrollArea;
    tab6scroll->setFrameShape(QFrame::NoFrame);
    tab6scroll->setWidgetResizable(true);
    auto* tab6 = new QWidget;
    tab6->setStyleSheet("background:#FAFAFA;");
    auto* t6lay = new QVBoxLayout(tab6);
    t6lay->setContentsMargins(16,16,16,16);
    t6lay->setSpacing(12);

    auto* consentInfo = new QLabel(
        "<b>Pristanak na intervenciju</b><br/>"
        "<span style='color:#555;font-size:11px;'>"
        "Priložite PDF pristanka ili pacijent potpiše direktno na tabletu."
        "</span>");
    consentInfo->setWordWrap(true);
    consentInfo->setStyleSheet("background:#F3E5F5;border:1px solid #E1BEE7;"
                               "border-radius:6px;padding:10px;");
    t6lay->addWidget(consentInfo);

    auto* consentDocGroup = makeGroup("Pristanak na intervenciju (PDF)");
    auto* cdLay = new QVBoxLayout(consentDocGroup);
    auto* cPdfRow = new QHBoxLayout;
    m_consentPdfPath = new QLineEdit;
    m_consentPdfPath->setPlaceholderText("Priložite PDF pristanka na intervenciju...");
    m_consentPdfPath->setReadOnly(true);
    m_consentPdfPath->setStyleSheet(fieldStyle);
    auto* browseConsentBtn = new QPushButton("📎 Priloži PDF");
    browseConsentBtn->setStyleSheet(
        "QPushButton{background:#6A1B9A;color:white;border-radius:4px;"
        "border:none;padding:6px 12px;font-weight:bold;}"
        "QPushButton:hover{background:#7B1FA2;}");
    auto* openConsentBtn = new QPushButton("🔍 Otvori");
    openConsentBtn->setStyleSheet(
        "QPushButton{background:#00695C;color:white;border-radius:4px;"
        "border:none;padding:6px 10px;font-weight:bold;}"
        "QPushButton:hover{background:#00897B;}");
    cPdfRow->addWidget(m_consentPdfPath, 1);
    cPdfRow->addWidget(browseConsentBtn);
    cPdfRow->addWidget(openConsentBtn);
    cdLay->addLayout(cPdfRow);
    t6lay->addWidget(consentDocGroup);

    auto* consentSigGroup = makeGroup("Potpis pacijenta na pristanak (tablet)");
    auto* cSigLay = new QVBoxLayout(consentSigGroup);
    auto* cSigInfo = new QLabel("Pacijent potpisuje direktno na ekranu olovkom ili prstom:");
    cSigInfo->setStyleSheet("color:#555;font-size:10px;");
    cSigLay->addWidget(cSigInfo);
    m_consentSignature = new SignatureWidget;
    cSigLay->addWidget(m_consentSignature);
    auto* clearConsentBtn = new QPushButton("🗑 Obriši potpis pristanka");
    clearConsentBtn->setStyleSheet(
        "QPushButton{background:#EF5350;color:white;border-radius:4px;"
        "border:none;padding:4px 10px;}"
        "QPushButton:hover{background:#E53935;}");
    auto* cBtnRow = new QHBoxLayout;
    cBtnRow->addStretch();
    cBtnRow->addWidget(clearConsentBtn);
    cSigLay->addLayout(cBtnRow);
    t6lay->addWidget(consentSigGroup);
    t6lay->addStretch();

    connect(browseConsentBtn, &QPushButton::clicked,
            this, &InterventionDialog::browseConsentPdf);
    connect(openConsentBtn, &QPushButton::clicked, [this](){
        if (!m_consentPdfPath->text().isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_consentPdfPath->text()));
    });
    connect(clearConsentBtn, &QPushButton::clicked,
            this, &InterventionDialog::clearConsentSignature);

    tab6scroll->setWidget(tab6);
    tabs->addTab(tab6scroll, "📝 Pristanak na intervenciju");

    mainLayout->addWidget(tabs, 1);

    // ── Buttons ───────────────────────────────────────────────────────────────
    auto* btnBar = new QWidget;
    btnBar->setStyleSheet("background:#EEEEEE; border-top:1px solid #E0E0E0;");
    auto* btnLay = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(24,12,24,12);
    btnLay->addStretch();

    auto* cancelBtn = new QPushButton("Odustani");
    cancelBtn->setMinimumSize(100,36);
    cancelBtn->setStyleSheet("QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
                             "background:white;color:#424242;font-weight:bold;}"
                             "QPushButton:hover{background:#F5F5F5;}");

    auto* saveBtn = new QPushButton(m_iv.id == 0 ? "Dodaj Intervenciju" : "Sačuvaj izmjene");
    saveBtn->setMinimumSize(160,36);
    saveBtn->setStyleSheet("QPushButton{background:#00695C;color:white;border-radius:4px;"
                           "border:none;font-weight:bold;}"
                           "QPushButton:hover{background:#00897B;}");

    btnLay->addWidget(cancelBtn);
    btnLay->addWidget(saveBtn);
    mainLayout->addWidget(btnBar);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn,   &QPushButton::clicked, this, &InterventionDialog::accept);
}

void InterventionDialog::onTypeChanged(const QString& /*text*/)
{
    // placeholder — moze se prosiriti npr. auto-popunjavanje
}

void InterventionDialog::populateFields()
{
    int ti = m_type->findText(m_iv.interventionType);
    if (ti >= 0) m_type->setCurrentIndex(ti);
    else         m_type->setEditText(m_iv.interventionType);

    m_tooth->setText(m_iv.tooth);
    m_product->setText(m_iv.product);

    int di = m_doctorCombo->findText(m_iv.doctor);
    if (di >= 0) m_doctorCombo->setCurrentIndex(di);
    else         m_doctorCombo->setEditText(m_iv.doctor);

    if (!m_iv.dateTime.isEmpty())
        m_dateTime->setDateTime(QDateTime::fromString(m_iv.dateTime,"yyyy-MM-dd HH:mm"));

    int si = m_status->findText(m_iv.status);
    if (si >= 0) m_status->setCurrentIndex(si);

    m_diagnosis->setPlainText(m_iv.diagnosis);
    m_treatment->setPlainText(m_iv.treatment);
    m_description->setPlainText(m_iv.description);
    m_notes->setPlainText(m_iv.notes);
    m_cost->setValue(m_iv.cost);
    int ci = m_currency->findText(m_iv.currency);
    if (ci >= 0) m_currency->setCurrentIndex(ci);
    m_pdfPath->setText(m_iv.pdfPath);

    if (!m_iv.signatureImagePath.isEmpty())
        m_signature->loadFromFile(m_iv.signatureImagePath);
    m_savedSigPath = m_iv.signatureImagePath;

    m_photoBeforePath = m_iv.photoBefore;
    m_photoAfterPath  = m_iv.photoAfter;
    updatePhotoPreview(m_photoBeforePrev, m_photoBeforePath);
    updatePhotoPreview(m_photoAfterPrev,  m_photoAfterPath);

    m_healthPdfPath->setText(m_iv.healthPdfPath);
    m_consentPdfPath->setText(m_iv.consentPdfPath);
    if (!m_iv.consentSignaturePath.isEmpty())
        m_consentSignature->loadFromFile(m_iv.consentSignaturePath);
    m_savedConsentSigPath = m_iv.consentSignaturePath;
}

void InterventionDialog::browsePdf()
{
    QString path = QFileDialog::getOpenFileName(this, "Priloži PDF dokument",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "PDF Dokumenti (*.pdf)");
    if (!path.isEmpty()) m_pdfPath->setText(path);
}

void InterventionDialog::clearSignature()
{
    m_signature->clear();
    m_savedSigPath.clear();
}

void InterventionDialog::browseHealthPdf()
{
    QString path = QFileDialog::getOpenFileName(this, "Priloži upitnik o zdravstvenom stanju",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "PDF Dokumenti (*.pdf)");
    if (!path.isEmpty()) m_healthPdfPath->setText(path);
}

void InterventionDialog::browseConsentPdf()
{
    QString path = QFileDialog::getOpenFileName(this, "Priloži PDF upitnika/pristanka",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "PDF Dokumenti (*.pdf)");
    if (!path.isEmpty()) m_consentPdfPath->setText(path);
}

void InterventionDialog::clearConsentSignature()
{
    m_consentSignature->clear();
    m_savedConsentSigPath.clear();
}

void InterventionDialog::browsePhotoBefore()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Fotografija PRIJE zahvata",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Slike (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/photos";
    QDir().mkpath(dir);
    QFileInfo fi(path);
    QString dest = dir + "/before_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "." + fi.suffix();
    QFile::copy(path, dest);
    m_photoBeforePath = dest;
    updatePhotoPreview(m_photoBeforePrev, m_photoBeforePath);
}

void InterventionDialog::browsePhotoAfter()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Fotografija POSLIJE zahvata",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Slike (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/photos";
    QDir().mkpath(dir);
    QFileInfo fi(path);
    QString dest = dir + "/after_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "." + fi.suffix();
    QFile::copy(path, dest);
    m_photoAfterPath = dest;
    updatePhotoPreview(m_photoAfterPrev, m_photoAfterPath);
}

void InterventionDialog::updatePhotoPreview(QLabel* lbl, const QString& path)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        lbl->setPixmap(QPixmap());
        lbl->setText("Nema fotografije");
        return;
    }
    QPixmap px(path);
    if (!px.isNull())
        lbl->setPixmap(px.scaled(220, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void InterventionDialog::accept()
{
    if (m_type->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Greška", "Vrsta intervencije je obavezna.");
        return;
    }

    m_iv.patientId        = m_patientId;
    m_iv.interventionType = m_type->currentText().trimmed();
    m_iv.tooth            = m_tooth->text().trimmed();
    m_iv.product          = m_product->text().trimmed();
    m_iv.doctor           = m_doctorCombo->currentText().trimmed();
    m_iv.dateTime         = m_dateTime->dateTime().toString("yyyy-MM-dd HH:mm");
    m_iv.status           = m_status->currentText();
    m_iv.diagnosis        = m_diagnosis->toPlainText().trimmed();
    m_iv.treatment        = m_treatment->toPlainText().trimmed();
    m_iv.description      = m_description->toPlainText().trimmed();
    m_iv.notes            = m_notes->toPlainText().trimmed();
    m_iv.cost             = m_cost->value();
    m_iv.currency         = m_currency->currentText();
    m_iv.pdfPath          = m_pdfPath->text().trimmed();
    m_iv.photoBefore      = m_photoBeforePath;
    m_iv.photoAfter       = m_photoAfterPath;
    m_iv.healthPdfPath    = m_healthPdfPath->text().trimmed();
    m_iv.consentPdfPath   = m_consentPdfPath->text().trimmed();

    // Sačuvaj potpis intervencije
    if (!m_signature->isEmpty()) {
        QString sigDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/signatures";
        QDir().mkpath(sigDir);
        QString sigPath = sigDir + "/sig_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
        if (m_signature->saveToFile(sigPath))
            m_iv.signatureImagePath = sigPath;
    } else {
        m_iv.signatureImagePath = m_savedSigPath;
    }

    // Sačuvaj potpis pristanka
    if (!m_consentSignature->isEmpty()) {
        QString sigDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/signatures";
        QDir().mkpath(sigDir);
        QString cSigPath = sigDir + "/consent_" +
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
        if (m_consentSignature->saveToFile(cSigPath))
            m_iv.consentSignaturePath = cSigPath;
    } else {
        m_iv.consentSignaturePath = m_savedConsentSigPath;
    }

    QDialog::accept();
}

Intervention InterventionDialog::getIntervention() const { return m_iv; }
