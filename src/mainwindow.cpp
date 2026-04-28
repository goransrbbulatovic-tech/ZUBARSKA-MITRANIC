#include "mainwindow.h"
#include "database.h"
#include "patientdialog.h"
#include "interventiondialog.h"
#include "patienthistorywidget.h"
#include "reportgenerator.h"
#include "clinicsettings.h"
#include "clinicsettingsdialog.h"
#include "statisticswidget.h"
#include "exportmanager.h"
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QTimer>
#include <QDateTime>

#include <QApplication>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QDateEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QFormLayout>
#include <QScrollArea>
#include <QFrame>
#include <QCheckBox>
#include <QFile>
#include <QGuiApplication>
#include <QClipboard>
#include <algorithm>
#include <QSizePolicy>
#include <QFont>

// ── Helper ──────────────────────────────────────────────────────────────────
static QTableWidgetItem* tItem(const QString& text, bool center = false) {
    auto* i = new QTableWidgetItem(text);
    i->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    if (center) i->setTextAlignment(Qt::AlignCenter);
    return i;
}

static QPushButton* makeActionBtn(const QString& label,
                                   const QString& bg,
                                   const QString& hover,
                                   QWidget* parent = nullptr)
{
    auto* b = new QPushButton(label, parent);
    b->setMinimumHeight(34);
    b->setStyleSheet(
        QString("QPushButton{background:%1;color:white;border-radius:4px;"
                "border:none;padding:0 14px;font-weight:bold;}"
                "QPushButton:hover{background:%2;}").arg(bg, hover));
    return b;
}

// ── Constructor ──────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    auto& cs = ClinicSettings::instance();
    setWindowTitle(cs.clinicName.isEmpty() ? "DentaPro" : cs.clinicName);
    setMinimumSize(1100, 720);
    resize(1300, 820);

    setupUi();
    loadPatientList();
    refreshDashboard();

    // Digitalni sat u status baru
    auto* clockLbl = new QLabel(this);
    clockLbl->setStyleSheet("color:#555;font-size:11px;padding:0 8px;");
    statusBar()->addPermanentWidget(clockLbl);
    statusBar()->showMessage("");

    auto updateClock = [clockLbl]() {
        clockLbl->setText(QDateTime::currentDateTime()
                              .toString("  dd.MM.yyyy   HH:mm:ss  "));
    };
    updateClock();
    auto* clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, updateClock);
    clockTimer->start(1000);
}

// ── setupUi ──────────────────────────────────────────────────────────────────
void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLay = new QHBoxLayout(central);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    setStyleSheet(
        "QMainWindow{background:#F5F5F5;}"
        "QMenuBar{background:#1565C0;color:white;font-size:11px;}"
        "QMenuBar::item{padding:6px 14px;}"
        "QMenuBar::item:selected{background:#1976D2;}"
        "QMenu{background:white;color:#212121;border:1px solid #E0E0E0;}"
        "QMenu::item:selected{background:#E3F2FD;color:#1565C0;}"
        "QToolBar{background:#1565C0;border:none;spacing:4px;padding:2px 6px;}"
        "QStatusBar{background:#F5F5F5;color:#757575;font-size:10px;"
        "border-top:1px solid #E0E0E0;}"
        "QSplitter::handle{background:#E0E0E0;}"
        "QScrollBar:vertical{border:none;background:#F5F5F5;width:8px;}"
        "QScrollBar::handle:vertical{background:#BDBDBD;border-radius:4px;min-height:30px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
    );

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(1);
    mainLay->addWidget(splitter);

    splitter->addWidget(buildLeftPanel());
    splitter->addWidget(buildRightPanel());
    splitter->setSizes({280, 1020});

    setupMenu();
    setupToolbar();
}

// ── Left panel ───────────────────────────────────────────────────────────────
QWidget* MainWindow::buildLeftPanel()
{
    auto* panel = new QWidget;
    panel->setFixedWidth(280);
    panel->setStyleSheet("background:#263238;border-right:1px solid #37474F;");

    auto* lay = new QVBoxLayout(panel);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Logo
    m_logoWidget = new QWidget;
    m_logoWidget->setStyleSheet("background:#1565C0;");
    auto* logoLay = new QVBoxLayout(m_logoWidget);
    logoLay->setContentsMargins(12, 12, 12, 12);
    logoLay->setSpacing(4);

    // Logo image label
    m_logoImageLbl = new QLabel;
    m_logoImageLbl->setAlignment(Qt::AlignCenter);
    m_logoImageLbl->setFixedHeight(60);
    m_logoImageLbl->setStyleSheet("background:transparent;border:none;");
    m_logoImageLbl->hide();

    // Clinic name text
    m_clinicNameLbl = new QLabel;
    m_clinicNameLbl->setStyleSheet(
        "color:white;font-size:18px;font-weight:bold;background:transparent;");
    m_clinicNameLbl->setWordWrap(true);

    m_clinicSubLbl = new QLabel;
    m_clinicSubLbl->setStyleSheet(
        "color:rgba(255,255,255,0.75);font-size:10px;background:transparent;");

    logoLay->addWidget(m_logoImageLbl);
    logoLay->addWidget(m_clinicNameLbl);
    logoLay->addWidget(m_clinicSubLbl);
    lay->addWidget(m_logoWidget);

    // Apply current branding
    applyClinicBranding();

    // Search box
    auto* sw = new QWidget;
    sw->setStyleSheet("background:#37474F;padding:8px;");
    auto* sLay = new QHBoxLayout(sw);
    sLay->setContentsMargins(8, 6, 8, 6);
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("Pretrazi pacijente...");
    m_searchBox->setStyleSheet(
        "QLineEdit{background:#455A64;border:none;border-radius:4px;"
        "padding:6px 10px;color:white;font-size:11px;}"
        "QLineEdit::placeholder{color:#90A4AE;}");
    sLay->addWidget(m_searchBox);
    lay->addWidget(sw);

    // Patient list
    m_patientList = new QListWidget;
    m_patientList->setStyleSheet(
        "QListWidget{background:#263238;border:none;outline:none;}"
        "QListWidget::item{color:#CFD8DC;padding:10px 16px;"
        "border-bottom:1px solid #37474F;font-size:12px;}"
        "QListWidget::item:selected{background:#1565C0;color:white;}"
        "QListWidget::item:hover:!selected{background:#37474F;}");
    lay->addWidget(m_patientList, 1);

    // Count label + Load More
    m_patientCountLbl = new QLabel;
    m_patientCountLbl->setStyleSheet(
        "color:#90A4AE;font-size:9px;padding:2px 12px;background:#263238;");
    m_patientCountLbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(m_patientCountLbl);

    m_loadMoreBtn = new QPushButton("Ucitaj jos pacijenata...");
    m_loadMoreBtn->setStyleSheet(
        "QPushButton{background:#37474F;color:#90A4AE;border:none;"
        "padding:6px;font-size:10px;}"
        "QPushButton:hover{background:#455A64;color:white;}");
    m_loadMoreBtn->hide();
    lay->addWidget(m_loadMoreBtn);

    // Add button
    auto* addBtn = new QPushButton("+ Novi Pacijent");
    addBtn->setStyleSheet(
        "QPushButton{background:#00695C;color:white;border:none;"
        "padding:12px;font-size:12px;font-weight:bold;}"
        "QPushButton:hover{background:#00897B;}");
    lay->addWidget(addBtn);

    connect(m_searchBox, &QLineEdit::textChanged, this, &MainWindow::onSearchPatients);
    connect(m_patientList, &QListWidget::itemClicked, this, &MainWindow::onPatientSelected);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddPatient);
    connect(m_loadMoreBtn, &QPushButton::clicked, this, &MainWindow::onLoadMorePatients);

    return panel;
}

// ── Right panel ───────────────────────────────────────────────────────────────
QWidget* MainWindow::buildRightPanel()
{
    auto* panel = new QWidget;
    panel->setStyleSheet("background:#F5F5F5;");

    auto* lay = new QVBoxLayout(panel);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    m_rightStack = new QStackedWidget;
    lay->addWidget(m_rightStack);

    setupDashboard();
    setupPatientsTab();
    setupSearchTab();
    setupAppointmentsTab();
    setupStatisticsTab();

    m_rightStack->setCurrentIndex(0);
    return panel;
}

// ── Dashboard ─────────────────────────────────────────────────────────────────
void MainWindow::setupDashboard()
{
    m_dashWidget = new QWidget;
    m_dashWidget->setStyleSheet("background:#F5F5F5;");
    auto* lay = new QVBoxLayout(m_dashWidget);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->setSpacing(20);

    auto& csRef = ClinicSettings::instance();
    QString welcomeText = csRef.clinicName.isEmpty()
        ? "Dobrodosli!"
        : "Dobrodosli u " + csRef.clinicName;
    m_welcomeLbl = new QLabel(welcomeText);
    auto* welcome = m_welcomeLbl;
    welcome->setStyleSheet("font-size:24px;font-weight:bold;color:#1565C0;");
    lay->addWidget(welcome);

    auto* sub = new QLabel("Sistem za upravljanje zubarskom ordinacijom");
    sub->setStyleSheet("font-size:12px;color:#757575;");
    lay->addWidget(sub);

    // Stat cards
    auto* row = new QHBoxLayout;
    row->setSpacing(16);

    auto makeCard = [&](const QString& label, QLabel*& val, const QString& bg) {
        auto* card = new QWidget;
        card->setStyleSheet(QString("QWidget{background:%1;border-radius:10px;}").arg(bg));
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(20, 16, 20, 16);
        val = new QLabel("...");
        val->setStyleSheet("font-size:28px;font-weight:bold;color:white;");
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("font-size:11px;color:rgba(255,255,255,0.8);");
        cl->addWidget(val);
        cl->addWidget(lbl);
        return card;
    };

    row->addWidget(makeCard("Ukupno pacijenata", m_statPatients, "#1565C0"));
    row->addWidget(makeCard("Intervencije ovog mjeseca", m_statIvMonth, "#00695C"));
    row->addWidget(makeCard("Prihod ovog mjeseca (BAM)", m_statRevMonth, "#E65100"));
    lay->addLayout(row);

    auto* apTitle = new QLabel("Nadolazeci termini (30 dana)");
    apTitle->setStyleSheet("font-size:15px;font-weight:bold;color:#1565C0;");
    lay->addWidget(apTitle);

    m_upcomingTable = new QTableWidget(0, 4);
    m_upcomingTable->setHorizontalHeaderLabels({"Pacijent", "Datum/Vrijeme", "Razlog", "Status"});
    m_upcomingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_upcomingTable->setStyleSheet(
        "QTableWidget{border:1px solid #E0E0E0;border-radius:6px;"
        "background:white;gridline-color:#F5F5F5;}"
        "QHeaderView::section{background:#1565C0;color:white;"
        "font-weight:bold;padding:8px;border:none;}");
    m_upcomingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_upcomingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_upcomingTable->setAlternatingRowColors(true);
    m_upcomingTable->verticalHeader()->setVisible(false);
    lay->addWidget(m_upcomingTable);

    // ── Nadolazeci rodjendani ─────────────────────────────────────────────────
    auto* bdTitle = new QLabel("🎂 Nadolazeci rođendani (7 dana)");
    bdTitle->setStyleSheet("font-size:14px;font-weight:bold;color:#C62828;margin-top:12px;");
    lay->addWidget(bdTitle);

    m_birthdayTable = new QTableWidget(0, 4);
    m_birthdayTable->setHorizontalHeaderLabels({"Pacijent", "Datum", "Tel/Viber", "Čestitka"});
    m_birthdayTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_birthdayTable->setStyleSheet(
        "QTableWidget{border:1px solid #E0E0E0;border-radius:8px;background:white;}"
        "QHeaderView::section{background:#C62828;color:white;font-weight:bold;padding:8px;border:none;}");
    m_birthdayTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_birthdayTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_birthdayTable->setAlternatingRowColors(true);
    m_birthdayTable->verticalHeader()->setVisible(false);
    m_birthdayTable->setMaximumHeight(180);
    lay->addWidget(m_birthdayTable);

    m_rightStack->addWidget(m_dashWidget); // index 0
}

// ── Patients tab ──────────────────────────────────────────────────────────────
void MainWindow::setupPatientsTab()
{
    m_detailWidget = new QWidget;
    m_detailWidget->setStyleSheet("background:#F5F5F5;");
    auto* lay = new QVBoxLayout(m_detailWidget);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Header bar
    auto* hBar = new QWidget;
    hBar->setStyleSheet("background:white;border-bottom:1px solid #E0E0E0;");
    auto* hLay = new QHBoxLayout(hBar);
    hLay->setContentsMargins(20, 12, 20, 12);

    m_patientNameLbl = new QLabel("Odaberite pacijenta");
    m_patientNameLbl->setStyleSheet("font-size:20px;font-weight:bold;color:#1565C0;");
    m_patientInfoLbl = new QLabel("");
    m_patientInfoLbl->setStyleSheet("font-size:11px;color:#757575;");

    auto* nameCol = new QVBoxLayout;
    nameCol->addWidget(m_patientNameLbl);
    nameCol->addWidget(m_patientInfoLbl);
    hLay->addLayout(nameCol, 1);

    auto* editPatBtn = makeActionBtn("Uredi pacijenta",   "#1565C0", "#1976D2");
    auto* delPatBtn  = makeActionBtn("Brisi pacijenta",   "#EF5350", "#E53935");
    auto* addIvBtn   = makeActionBtn("+ Nova Intervencija","#00695C", "#00897B");
    auto* printBtn   = makeActionBtn("Stampaj Izvjestaj", "#E65100", "#EF6C00");

    hLay->addWidget(editPatBtn);
    hLay->addWidget(delPatBtn);
    hLay->addSpacing(12);
    hLay->addWidget(addIvBtn);
    hLay->addWidget(printBtn);
    lay->addWidget(hBar);

    // Tabs
    m_detailTabs = new QTabWidget;
    m_detailTabs->setStyleSheet(
        "QTabWidget::pane{border:none;background:#F5F5F5;}"
        "QTabBar::tab{padding:10px 20px;font-size:11px;font-weight:bold;"
        "background:#EEEEEE;color:#757575;border:none;margin-right:2px;}"
        "QTabBar::tab:selected{background:#F5F5F5;color:#1565C0;"
        "border-bottom:3px solid #1565C0;}"
        "QTabBar::tab:hover:!selected{background:#E3F2FD;color:#1565C0;}");
    lay->addWidget(m_detailTabs, 1);

    // Tab 1 - Overview
    auto* ov = new QWidget;
    ov->setStyleSheet("background:#F5F5F5;");
    auto* ovLay = new QVBoxLayout(ov);
    ovLay->setContentsMargins(20, 16, 20, 16);

    auto* ivTitle = new QLabel("Sve intervencije");
    ivTitle->setStyleSheet("font-size:15px;font-weight:bold;color:#00695C;");
    ovLay->addWidget(ivTitle);

    m_allIvTable = new QTableWidget(0, 6);
    m_allIvTable->setHorizontalHeaderLabels({"Datum","Vrsta","Zub","Doktor","Cijena","Status"});
    m_allIvTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_allIvTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_allIvTable->setStyleSheet(
        "QTableWidget{border:1px solid #E0E0E0;border-radius:6px;"
        "background:white;gridline-color:#F5F5F5;}"
        "QHeaderView::section{background:#00695C;color:white;"
        "font-weight:bold;padding:8px;border:none;}");
    m_allIvTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_allIvTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_allIvTable->setAlternatingRowColors(true);
    m_allIvTable->verticalHeader()->setVisible(false);
    ovLay->addWidget(m_allIvTable);

    // Paginacija intervencija
    m_ivCountLbl = new QLabel;
    m_ivCountLbl->setStyleSheet(
        "color:#9E9E9E;font-size:9px;padding:2px 4px;");
    m_ivCountLbl->setAlignment(Qt::AlignCenter);
    ovLay->addWidget(m_ivCountLbl);

    m_ivLoadMoreBtn = new QPushButton("Ucitaj starije intervencije...");
    m_ivLoadMoreBtn->setStyleSheet(
        "QPushButton{background:#E8F5E9;color:#00695C;border:1px solid #C8E6C9;"
        "border-radius:4px;padding:6px;font-size:10px;font-weight:bold;}"
        "QPushButton:hover{background:#C8E6C9;}");
    m_ivLoadMoreBtn->hide();
    ovLay->addWidget(m_ivLoadMoreBtn);
    connect(m_ivLoadMoreBtn, &QPushButton::clicked,
            this, &MainWindow::onLoadMoreInterventions);

    m_detailTabs->addTab(ov, "Pregled");

    // Tab 2 - History
    m_historyWidget = new PatientHistoryWidget;
    m_detailTabs->addTab(m_historyWidget, "Intervencije");

    connect(m_historyWidget, &PatientHistoryWidget::editIntervention,
            this, &MainWindow::onEditIntervention);
    connect(m_historyWidget, &PatientHistoryWidget::deleteIntervention,
            this, &MainWindow::onDeleteIntervention);
    connect(m_historyWidget, &PatientHistoryWidget::viewPdf,
            this, &MainWindow::onViewPdf);
    connect(editPatBtn, &QPushButton::clicked, this, &MainWindow::onEditPatient);
    connect(delPatBtn,  &QPushButton::clicked, this, &MainWindow::onDeletePatient);
    connect(addIvBtn,   &QPushButton::clicked, this, &MainWindow::onAddIntervention);
    connect(printBtn,   &QPushButton::clicked, this, &MainWindow::onPrintPatientReport);

    m_rightStack->addWidget(m_detailWidget); // index 1
}

// ── Search tab ────────────────────────────────────────────────────────────────
void MainWindow::setupSearchTab()
{
    m_searchWidget = new QWidget;
    m_searchWidget->setStyleSheet("background:#F5F5F5;");
    auto* lay = new QVBoxLayout(m_searchWidget);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->setSpacing(16);

    auto* title = new QLabel("Napredna Pretraga");
    title->setStyleSheet("font-size:22px;font-weight:bold;color:#1565C0;");
    lay->addWidget(title);

    auto* fc = new QWidget;
    fc->setStyleSheet("background:white;border-radius:8px;border:1px solid #E0E0E0;");
    auto* fLay = new QHBoxLayout(fc);
    fLay->setContentsMargins(16, 12, 16, 12);
    fLay->setSpacing(12);

    const QString fs =
        "QLineEdit,QDateEdit,QComboBox{"
        "border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;min-height:30px;}"
        "QLineEdit:focus,QDateEdit:focus,QComboBox:focus{border:2px solid #1565C0;}";

    m_searchName = new QLineEdit;
    m_searchName->setPlaceholderText("Ime ili prezime");
    m_searchName->setStyleSheet(fs);

    m_searchFrom = new QDateEdit(QDate::currentDate().addMonths(-6));
    m_searchFrom->setCalendarPopup(true);
    m_searchFrom->setDisplayFormat("dd.MM.yyyy");
    m_searchFrom->setStyleSheet(fs);

    m_searchTo = new QDateEdit(QDate::currentDate());
    m_searchTo->setCalendarPopup(true);
    m_searchTo->setDisplayFormat("dd.MM.yyyy");
    m_searchTo->setStyleSheet(fs);

    m_searchType = new QComboBox;
    m_searchType->addItems({"Sve vrste","Pregled","Vadjenje zuba","Plomba",
                             "Devitalizacija","Protetika","Ortodoncija",
                             "Rengen","Ciscenje kamenca","Implant"});
    m_searchType->setStyleSheet(fs);

    auto* searchBtn = makeActionBtn("Pretrazi", "#1565C0", "#1976D2");
    auto* resetBtn  = new QPushButton("Reset");
    resetBtn->setMinimumHeight(34);
    resetBtn->setStyleSheet(
        "QPushButton{background:white;color:#1565C0;border-radius:4px;"
        "border:1px solid #1565C0;padding:0 12px;font-weight:bold;}"
        "QPushButton:hover{background:#E3F2FD;}");

    fLay->addWidget(new QLabel("Pacijent:"));
    fLay->addWidget(m_searchName, 2);
    fLay->addWidget(new QLabel("Od:"));
    fLay->addWidget(m_searchFrom);
    fLay->addWidget(new QLabel("Do:"));
    fLay->addWidget(m_searchTo);
    fLay->addWidget(new QLabel("Vrsta:"));
    fLay->addWidget(m_searchType, 1);
    fLay->addWidget(searchBtn);
    fLay->addWidget(resetBtn);
    lay->addWidget(fc);

    m_searchResults = new QTableWidget(0, 7);
    m_searchResults->setHorizontalHeaderLabels(
        {"Pacijent","Datum","Vrsta","Zub","Doktor","Cijena","Status"});
    m_searchResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_searchResults->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_searchResults->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_searchResults->setStyleSheet(
        "QTableWidget{border:1px solid #E0E0E0;border-radius:8px;"
        "background:white;gridline-color:#F5F5F5;}"
        "QHeaderView::section{background:#1565C0;color:white;"
        "font-weight:bold;padding:8px;border:none;}");
    m_searchResults->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_searchResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_searchResults->setAlternatingRowColors(true);
    m_searchResults->verticalHeader()->setVisible(false);
    lay->addWidget(m_searchResults, 1);

    auto* hint = new QLabel("Dvaput kliknite na red da vidite detalje pacijenta");
    hint->setStyleSheet("font-size:10px;color:#9E9E9E;");
    lay->addWidget(hint);

    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(resetBtn, &QPushButton::clicked, [this](){
        m_searchName->clear();
        m_searchResults->setRowCount(0);
        m_searchIvs.clear();
    });
    connect(m_searchResults, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::onSearchResultDoubleClicked);

    m_rightStack->addWidget(m_searchWidget); // index 2
}

// ── Appointments tab ──────────────────────────────────────────────────────────
void MainWindow::setupAppointmentsTab()
{
    m_appointmentsWidget = new QWidget;
    m_appointmentsWidget->setStyleSheet("background:#F5F5F5;");
    auto* lay = new QVBoxLayout(m_appointmentsWidget);
    lay->setContentsMargins(24, 24, 24, 24);
    lay->setSpacing(16);

    // Header row
    auto* hLay = new QHBoxLayout;
    auto* title = new QLabel("Zakazani Termini");
    title->setStyleSheet("font-size:22px;font-weight:bold;color:#1565C0;");
    hLay->addWidget(title, 1);

    // Filter dugmad
    auto makeFilter = [&](const QString& txt, const QString& col,
                           const QString& hov) -> QPushButton* {
        auto* b = new QPushButton(txt);
        b->setCheckable(true);
        b->setStyleSheet(
            QString("QPushButton{background:%1;color:white;border:none;"
                    "border-radius:4px;padding:6px 14px;font-weight:bold;}"
                    "QPushButton:hover{background:%2;}"
                    "QPushButton:checked{border:2px solid white;}").arg(col, hov));
        return b;
    };
    auto* btnSvi      = makeFilter("Svi",       "#607D8B", "#546E7A");
    auto* btnZakazani = makeFilter("Zakazani",  "#00695C", "#00897B");
    auto* btnZavrseni = makeFilter("Zavrseni",  "#1565C0", "#1976D2");
    auto* btnOtkazani = makeFilter("Otkazani",  "#C62828", "#E53935");
    btnZakazani->setChecked(true);
    hLay->addWidget(btnSvi);
    hLay->addWidget(btnZakazani);
    hLay->addWidget(btnZavrseni);
    hLay->addWidget(btnOtkazani);
    hLay->addSpacing(8);
    auto* addApBtn = makeActionBtn("+ Zakazi Termin", "#00695C", "#00897B");
    hLay->addWidget(addApBtn);
    lay->addLayout(hLay);

    // Table
    m_appointTable = new QTableWidget(0, 6);
    m_appointTable->setHorizontalHeaderLabels(
        {"Pacijent", "Datum/Vrijeme", "Razlog", "Status", "Napomena", "Akcija"});
    m_appointTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_appointTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_appointTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_appointTable->setStyleSheet(
        "QTableWidget{border:1px solid #E0E0E0;border-radius:8px;"
        "background:white;gridline-color:#F5F5F5;}"
        "QHeaderView::section{background:#1565C0;color:white;"
        "font-weight:bold;padding:8px;border:none;}");
    m_appointTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_appointTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_appointTable->setAlternatingRowColors(true);
    m_appointTable->verticalHeader()->setVisible(false);
    lay->addWidget(m_appointTable, 1);

    connect(addApBtn, &QPushButton::clicked, this, &MainWindow::onAddAppointment);

    // Filter connections
    auto applyFilter = [this, btnSvi, btnZakazani, btnZavrseni, btnOtkazani]
                        (QPushButton* active, const QString& filter) {
        btnSvi->setChecked(false);
        btnZakazani->setChecked(false);
        btnZavrseni->setChecked(false);
        btnOtkazani->setChecked(false);
        active->setChecked(true);
        refreshAppointments(filter);
    };

    connect(btnSvi,      &QPushButton::clicked, [=](){ applyFilter(btnSvi,      ""); });
    connect(btnZakazani, &QPushButton::clicked, [=](){ applyFilter(btnZakazani, "scheduled"); });
    connect(btnZavrseni, &QPushButton::clicked, [=](){ applyFilter(btnZavrseni, "completed"); });
    connect(btnOtkazani, &QPushButton::clicked, [=](){ applyFilter(btnOtkazani, "cancelled"); });

    m_rightStack->addWidget(m_appointmentsWidget); // index 3
}

// ── Menu ──────────────────────────────────────────────────────────────────────
void MainWindow::setupMenu()
{
    auto* mb = menuBar();

    auto* fileMenu = mb->addMenu("Fajl");
    fileMenu->addAction("Novi pacijent", this, &MainWindow::onAddPatient,
                        QKeySequence::New);
    fileMenu->addSeparator();
    fileMenu->addAction("Izlaz", qApp, &QApplication::quit, QKeySequence::Quit);

    auto* viewMenu = mb->addMenu("Prikaz");
    viewMenu->addAction("Dashboard", [this](){
        m_rightStack->setCurrentIndex(0);
        refreshDashboard();
    });
    viewMenu->addAction("Pacijenti", [this](){
        if (m_currentPatient.id > 0) m_rightStack->setCurrentIndex(1);
    });
    viewMenu->addAction("Pretraga", [this](){
        m_rightStack->setCurrentIndex(2);
    });
    viewMenu->addAction("Termini", [this](){
        m_rightStack->setCurrentIndex(3);
        refreshDashboard();
    });

    auto* repMenu = mb->addMenu("Izvjestaji");
    repMenu->addAction("Stampaj izvjestaj pacijenta",
                       this, &MainWindow::onPrintPatientReport);
    repMenu->addSeparator();
    repMenu->addSeparator();
    repMenu->addAction("📊 Izvezi pacijenta u Excel (XLSX)",
                       this, &MainWindow::onExportPatientXlsx);
    repMenu->addAction("📊 Izvezi kompletan izvještaj u Excel (XLSX)",
                       this, &MainWindow::onExportFullXlsx);
    repMenu->addSeparator();
    repMenu->addAction("📄 Izvezi pacijenta u PDF",
                       this, &MainWindow::onExportPatientPdf);
    repMenu->addAction("📄 Izvezi kompletan izvještaj u PDF",
                       this, &MainWindow::onExportFullPdf);
    repMenu->addSeparator();
    repMenu->addAction("Izvezi pacijenta u CSV",
                       this, &MainWindow::onExportPatientCsv);
    repMenu->addAction("Izvezi kompletan izvjestaj u CSV",
                       this, &MainWindow::onExportFullCsv);

    // ── Settings menu ─────────────────────────────────────────────────────
    auto* settMenu = mb->addMenu("Podesavanja");
    settMenu->addAction("Ordinacija (logo, naziv, doktori)",
                        this, &MainWindow::onOpenClinicSettings);
    settMenu->addSeparator();
    settMenu->addAction("Backup baze podataka",
                        this, &MainWindow::onBackupDatabase);

    auto* helpMenu = mb->addMenu("Pomoc");
    helpMenu->addAction("O programu", [this](){
        auto& cs = ClinicSettings::instance();
        QString naziv = cs.clinicName.isEmpty() ? "Stomatoloska ordinacija" : cs.clinicName;
        QMessageBox::about(this, "O programu",
            "<b>" + naziv + " v1.0</b><br/>"
            "Sistem za upravljanje zubarskom ordinacijom<br/><br/>"
            "Razvijeno u C++ / Qt6<br/>"
            "<small>2026 " + naziv + "</small>");
    });
}

// ── Toolbar ───────────────────────────────────────────────────────────────────
void MainWindow::setupToolbar()
{
    auto* tb = addToolBar("Toolbar");
    tb->setMovable(false);

    const QString btnStyle =
        "QPushButton{background:rgba(255,255,255,0.15);color:white;"
        "border:1px solid rgba(255,255,255,0.3);border-radius:4px;"
        "padding:5px 12px;font-size:11px;font-weight:bold;}"
        "QPushButton:hover{background:rgba(255,255,255,0.25);}";

    auto addBtn = [&](const QString& lbl, std::function<void()> fn) {
        auto* b = new QPushButton(lbl);
        b->setStyleSheet(btnStyle);
        connect(b, &QPushButton::clicked, fn);
        tb->addWidget(b);
    };

    addBtn("Dashboard",         [this](){ m_rightStack->setCurrentIndex(0); refreshDashboard(); });
    tb->addSeparator();
    addBtn("Novi Pacijent",     [this](){ onAddPatient(); });
    addBtn("Nova Intervencija", [this](){ onAddIntervention(); });
    tb->addSeparator();
    addBtn("Pretraga",          [this](){ m_rightStack->setCurrentIndex(2); });
    addBtn("Termini",           [this](){ m_rightStack->setCurrentIndex(3); refreshAppointments(); });
    addBtn("Statistike",        [this](){ m_rightStack->setCurrentIndex(4);
                                          if (m_statsWidget) m_statsWidget->refresh(); });
    tb->addSeparator();
    addBtn("Stampaj",           [this](){ onPrintPatientReport(); });
    addBtn("Backup",            [this](){ onBackupDatabase(); });
    tb->addSeparator();
    addBtn("Podesavanja",       [this](){ onOpenClinicSettings(); });

    auto* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background:transparent;");
    tb->addWidget(spacer);

    auto* dateLbl = new QLabel(QDate::currentDate().toString("dddd, dd. MMMM yyyy."));
    dateLbl->setStyleSheet("color:rgba(255,255,255,0.8);font-size:10px;padding:0 12px;");
    tb->addWidget(dateLbl);
}

// ── Load / Refresh ─────────────────────────────────────────────────────────────
void MainWindow::loadPatientList()
{
    m_patientOffset = 0;
    m_lastSearch    = m_searchBox->text().trimmed();
    m_patientTotal  = Database::instance().countPatients(m_lastSearch);

    m_patientList->clear();
    auto patients = Database::instance().getPatientsPage(
        0, PAGE_SIZE, m_lastSearch);

    for (const auto& p : patients) {
        auto* item = new QListWidgetItem(p.fullName());
        item->setData(Qt::UserRole, p.id);
        m_patientList->addItem(item);
    }
    m_patientOffset = patients.size();

    // Update count label
    m_patientCountLbl->setText(
        QString("Prikazano %1 od %2")
            .arg(m_patientOffset).arg(m_patientTotal));

    // Show/hide Load More button
    m_loadMoreBtn->setVisible(m_patientOffset < m_patientTotal);
    if (m_loadMoreBtn->isVisible())
        m_loadMoreBtn->setText(
            QString("Ucitaj jos (%1 preostalo)...")
                .arg(m_patientTotal - m_patientOffset));

    statusBar()->showMessage(
        QString("Pacijenata: %1").arg(m_patientTotal));
}

void MainWindow::onLoadMorePatients()
{
    auto patients = Database::instance().getPatientsPage(
        m_patientOffset, PAGE_SIZE, m_lastSearch);

    for (const auto& p : patients) {
        auto* item = new QListWidgetItem(p.fullName());
        item->setData(Qt::UserRole, p.id);
        m_patientList->addItem(item);
    }
    m_patientOffset += patients.size();

    m_patientCountLbl->setText(
        QString("Prikazano %1 od %2")
            .arg(m_patientOffset).arg(m_patientTotal));

    m_loadMoreBtn->setVisible(m_patientOffset < m_patientTotal);
    if (m_loadMoreBtn->isVisible())
        m_loadMoreBtn->setText(
            QString("Ucitaj jos (%1 preostalo)...")
                .arg(m_patientTotal - m_patientOffset));
}

void MainWindow::refreshDashboard()
{
    m_statPatients->setText(
        QString::number(Database::instance().totalPatients()));
    m_statIvMonth->setText(
        QString::number(Database::instance().totalInterventionsThisMonth()));
    m_statRevMonth->setText(
        QString::number(Database::instance().totalRevenueThisMonth(), 'f', 2));

    auto apts = Database::instance().getUpcomingAppointments(30);
    m_upcomingTable->setRowCount(apts.size());
    for (int i = 0; i < apts.size(); ++i) {
        const auto& ap = apts[i];
        auto p = Database::instance().getPatient(ap.patientId);
        QString dt = ap.dateTime;
        if (dt.length() >= 16)
            dt = QDateTime::fromString(dt,"yyyy-MM-dd HH:mm")
                     .toString("dd.MM.yyyy HH:mm");
        m_upcomingTable->setItem(i, 0, tItem(p.fullName()));
        m_upcomingTable->setItem(i, 1, tItem(dt, true));
        m_upcomingTable->setItem(i, 2, tItem(ap.reason));
        m_upcomingTable->setItem(i, 3, tItem(ap.status, true));
    }

    // ── Rodjendani u narednih 7 dana ─────────────────────────────────────────
    QDate today = QDate::currentDate();
    auto allPatients = Database::instance().getAllPatients();
    QList<QPair<QDate,Patient>> upcoming;

    for (const auto& p : allPatients) {
        if (p.dateOfBirth.isEmpty()) continue;
        QDate dob = QDate::fromString(p.dateOfBirth, "yyyy-MM-dd");
        if (!dob.isValid()) continue;

        // Rodjendan ove godine
        QDate bdThisYear(today.year(), dob.month(), dob.day());
        // Ako je presao — provjeri iducu godinu
        if (bdThisYear < today)
            bdThisYear = QDate(today.year()+1, dob.month(), dob.day());

        int daysUntil = today.daysTo(bdThisYear);
        if (daysUntil <= 7)
            upcoming.append({bdThisYear, p});
    }

    // Sortiraj po datumu
    std::sort(upcoming.begin(), upcoming.end(),
              [](const auto& a, const auto& b){ return a.first < b.first; });

    auto& cs = ClinicSettings::instance();
    m_birthdayTable->setRowCount(upcoming.size());

    for (int i = 0; i < upcoming.size(); ++i) {
        const auto& [bdDate, p] = upcoming[i];
        int daysLeft = today.daysTo(bdDate);

        QString dateStr = bdDate.toString("dd.MM.yyyy.");
        if (daysLeft == 0)      dateStr = "🎂 DANAS!";
        else if (daysLeft == 1) dateStr = "🎂 Sutra";
        else                    dateStr = QString("Za %1 dana").arg(daysLeft);

        m_birthdayTable->setItem(i, 0, tItem(p.fullName()));
        m_birthdayTable->setItem(i, 1, tItem(dateStr, true));
        m_birthdayTable->setItem(i, 2, tItem(p.phone, true));

        // Dugme za cestitku
        auto* btn = new QPushButton("🎂 Čestitaj");
        btn->setStyleSheet(
            "QPushButton{background:#C62828;color:white;border:none;"
            "border-radius:3px;padding:3px 8px;font-size:10px;font-weight:bold;}"
            "QPushButton:hover{background:#E53935;}");

        connect(btn, &QPushButton::clicked, [this, p, cs](){
            QString msg = QString("Poštovani/a %1,\n\nČestitamo Vam sretan rođendan! 🎂🦷\n\n"
                                  "Srdačni pozdrav,\n%2")
                          .arg(p.firstName, cs.clinicName);

            QDialog dlg(this);
            dlg.setWindowTitle("🎂 Čestitka za rođendan");
            dlg.setFixedWidth(400);
            auto* lay = new QVBoxLayout(&dlg);
            lay->setContentsMargins(20,20,20,20);
            lay->setSpacing(12);

            auto* nameLbl = new QLabel(QString("🎂  <b>%1</b>").arg(p.fullName()));
            nameLbl->setStyleSheet("font-size:13px;color:#C62828;");
            lay->addWidget(nameLbl);

            auto* msgEdit = new QTextEdit;
            msgEdit->setPlainText(msg);
            msgEdit->setStyleSheet(
                "QTextEdit{border:1px solid #BDBDBD;border-radius:4px;padding:6px;"
                "font-size:11px;background:#FFFDE7;}");
            msgEdit->setFixedHeight(130);
            lay->addWidget(msgEdit);

            auto* hintLbl = new QLabel("Možeš urediti tekst čestitke prije slanja.");
            hintLbl->setStyleSheet("color:#9E9E9E;font-size:9px;");
            lay->addWidget(hintLbl);

            auto* btnRow = new QHBoxLayout;

            auto* copyBtn = new QPushButton("📋 Kopiraj");
            copyBtn->setStyleSheet(
                "QPushButton{background:#1565C0;color:white;border-radius:4px;"
                "border:none;padding:6px 16px;font-weight:bold;}"
                "QPushButton:hover{background:#1976D2;}");

            auto* viberBtn = new QPushButton("📱 Otvori Viber");
            viberBtn->setStyleSheet(
                "QPushButton{background:#7B1FA2;color:white;border-radius:4px;"
                "border:none;padding:6px 16px;font-weight:bold;}"
                "QPushButton:hover{background:#8E24AA;}");

            auto* closeBtn = new QPushButton("Zatvori");
            closeBtn->setStyleSheet(
                "QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
                "background:white;padding:6px 14px;}"
                "QPushButton:hover{background:#F5F5F5;}");

            btnRow->addWidget(copyBtn);
            btnRow->addWidget(viberBtn);
            btnRow->addStretch();
            btnRow->addWidget(closeBtn);
            lay->addLayout(btnRow);

            connect(copyBtn, &QPushButton::clicked, [&](){
                QGuiApplication::clipboard()->setText(msgEdit->toPlainText());
                copyBtn->setText("✅ Kopirano!");
                copyBtn->setStyleSheet(
                    "QPushButton{background:#2E7D32;color:white;border-radius:4px;"
                    "border:none;padding:6px 16px;font-weight:bold;}");
            });

            connect(viberBtn, &QPushButton::clicked, [&](){
                QString phone = p.phone;
                phone.remove(" ").remove("-").remove("(").remove(")");
                if (!phone.isEmpty()) {
                    // Otvori Viber chat sa pacijentom
                    QDesktopServices::openUrl(
                        QUrl("viber://chat?number=" + phone));
                } else {
                    QMessageBox::information(&dlg, "Viber",
                        "Pacijent nema unesen broj telefona.");
                }
            });

            connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

            dlg.exec();
        });

        m_birthdayTable->setCellWidget(i, 3, btn);

        // Istakni ako je danas
        if (daysLeft == 0) {
            for (int c = 0; c < 3; ++c)
                if (m_birthdayTable->item(i, c))
                    m_birthdayTable->item(i, c)->setBackground(QColor("#FFEBEE"));
        }
    }

    if (upcoming.isEmpty()) {
        m_birthdayTable->insertRow(0);
        auto* noItem = new QTableWidgetItem("Nema rodjendana u narednih 7 dana.");
        noItem->setTextAlignment(Qt::AlignCenter);
        noItem->setForeground(QColor("#9E9E9E"));
        m_birthdayTable->setItem(0, 0, noItem);
        m_birthdayTable->setSpan(0, 0, 1, 4);
    }

    m_rightStack->setCurrentIndex(0);
}

void MainWindow::selectPatientById(int id)
{
    for (int i = 0; i < m_patientList->count(); ++i) {
        if (m_patientList->item(i)->data(Qt::UserRole).toInt() == id) {
            m_patientList->setCurrentRow(i);
            break;
        }
    }
}

void MainWindow::refreshPatientDetail()
{
    if (m_currentPatient.id == 0) return;
    m_currentPatient = Database::instance().getPatient(m_currentPatient.id);
    showPatientCard(m_currentPatient);
    m_historyWidget->setPatient(m_currentPatient);

    // Paginirano ucitavanje intervencija — samo prvih IV_PAGE_SIZE
    m_ivOffset = 0;
    m_ivTotal  = Database::instance().countPatientInterventions(m_currentPatient.id);

    auto ivs = Database::instance().getPatientInterventionsPage(
        m_currentPatient.id, 0, IV_PAGE_SIZE);

    m_allIvTable->setRowCount(ivs.size());
    for (int i = 0; i < ivs.size(); ++i) {
        const auto& iv = ivs[i];
        QString dt = iv.dateTime.length() >= 10 ?
            QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd")
                .toString("dd.MM.yyyy.") : iv.dateTime;
        m_allIvTable->setItem(i, 0, tItem(dt, true));
        m_allIvTable->setItem(i, 1, tItem(iv.interventionType));
        m_allIvTable->setItem(i, 2, tItem(iv.tooth, true));
        m_allIvTable->setItem(i, 3, tItem(iv.doctor, true));
        QString cijena = QString("%1 %2").arg(iv.cost, 0, 'f', 2).arg(iv.currency);
        m_allIvTable->setItem(i, 4, tItem(cijena, true));
        QString status = iv.status == "completed" ? "Zavrseno" :
                         iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
        m_allIvTable->setItem(i, 5, tItem(status, true));
    }
    m_ivOffset = ivs.size();

    // Paginacija label i dugme
    m_ivCountLbl->setText(
        QString("Prikazano %1 od %2 intervencija")
            .arg(m_ivOffset).arg(m_ivTotal));
    m_ivLoadMoreBtn->setVisible(m_ivOffset < m_ivTotal);
    if (m_ivLoadMoreBtn->isVisible())
        m_ivLoadMoreBtn->setText(
            QString("Ucitaj starije intervencije (%1 preostalo)...")
                .arg(m_ivTotal - m_ivOffset));
}

void MainWindow::onLoadMoreInterventions()
{
    if (m_currentPatient.id == 0) return;

    auto ivs = Database::instance().getPatientInterventionsPage(
        m_currentPatient.id, m_ivOffset, IV_PAGE_SIZE);

    int startRow = m_allIvTable->rowCount();
    m_allIvTable->setRowCount(startRow + ivs.size());

    for (int i = 0; i < ivs.size(); ++i) {
        const auto& iv = ivs[i];
        QString dt = iv.dateTime.length() >= 10 ?
            QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd")
                .toString("dd.MM.yyyy.") : iv.dateTime;
        int row = startRow + i;
        m_allIvTable->setItem(row, 0, tItem(dt, true));
        m_allIvTable->setItem(row, 1, tItem(iv.interventionType));
        m_allIvTable->setItem(row, 2, tItem(iv.tooth, true));
        m_allIvTable->setItem(row, 3, tItem(iv.doctor, true));
        QString cijena = QString("%1 %2").arg(iv.cost, 0, 'f', 2).arg(iv.currency);
        m_allIvTable->setItem(row, 4, tItem(cijena, true));
        QString status = iv.status == "completed" ? "Zavrseno" :
                         iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
        m_allIvTable->setItem(row, 5, tItem(status, true));
    }
    m_ivOffset += ivs.size();

    m_ivCountLbl->setText(
        QString("Prikazano %1 od %2 intervencija")
            .arg(m_ivOffset).arg(m_ivTotal));
    m_ivLoadMoreBtn->setVisible(m_ivOffset < m_ivTotal);
    if (m_ivLoadMoreBtn->isVisible())
        m_ivLoadMoreBtn->setText(
            QString("Ucitaj starije intervencije (%1 preostalo)...")
                .arg(m_ivTotal - m_ivOffset));
}

void MainWindow::showPatientCard(const Patient& p)
{
    m_patientNameLbl->setText(p.fullName());
    QString info;
    if (!p.dateOfBirth.isEmpty())
        info += QDate::fromString(p.dateOfBirth,"yyyy-MM-dd")
                    .toString("dd.MM.yyyy.") + "  ";
    if (!p.phone.isEmpty())  info += p.phone + "  ";
    if (!p.email.isEmpty())  info += p.email + "  ";
    if (!p.bloodType.isEmpty() && p.bloodType != "Nepoznata")
        info += "Krvna gr: " + p.bloodType;
    if (!p.allergies.isEmpty())
        info += "  Alergije: " + p.allergies;
    m_patientInfoLbl->setText(info);
}

// ── Patient slots ─────────────────────────────────────────────────────────────
void MainWindow::onPatientSelected(QListWidgetItem* item)
{
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    m_currentPatient = Database::instance().getPatient(id);
    refreshPatientDetail();
    m_rightStack->setCurrentIndex(1);
}

void MainWindow::onSearchPatients(const QString&)
{
    loadPatientList();
}

void MainWindow::onAddPatient()
{
    PatientDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        Patient p = dlg.getPatient();
        if (Database::instance().addPatient(p)) {
            loadPatientList();
            m_currentPatient = p;
            refreshPatientDetail();
            selectPatientById(p.id);
            m_rightStack->setCurrentIndex(1);
            statusBar()->showMessage("Pacijent uspjesno dodan.", 3000);
        }
    }
}

void MainWindow::onEditPatient()
{
    if (m_currentPatient.id == 0) {
        QMessageBox::information(this, "Napomena", "Odaberite pacijenta iz liste.");
        return;
    }
    PatientDialog dlg(this, m_currentPatient);
    if (dlg.exec() == QDialog::Accepted) {
        Patient p = dlg.getPatient();
        if (Database::instance().updatePatient(p)) {
            loadPatientList();
            selectPatientById(p.id);
            m_currentPatient = p;
            refreshPatientDetail();
            statusBar()->showMessage("Podaci pacijenta azurirani.", 3000);
        }
    }
}

void MainWindow::onDeletePatient()
{
    if (m_currentPatient.id == 0) return;
    auto reply = QMessageBox::question(this, "Brisanje pacijenta",
        QString("Da li ste sigurni da zelite obrisati pacijenta '%1'?\n"
                "Sve intervencije i termini ce biti obrisani.")
                .arg(m_currentPatient.fullName()),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        Database::instance().deletePatient(m_currentPatient.id);
        m_currentPatient = {};
        loadPatientList();
        m_rightStack->setCurrentIndex(0);
        refreshDashboard();
        statusBar()->showMessage("Pacijent obrisan.", 3000);
    }
}

// ── Intervention slots ────────────────────────────────────────────────────────
void MainWindow::onAddIntervention()
{
    if (m_currentPatient.id == 0) {
        QMessageBox::information(this, "Napomena", "Odaberite pacijenta iz liste.");
        return;
    }
    InterventionDialog dlg(this, m_currentPatient.id);
    if (dlg.exec() == QDialog::Accepted) {
        Intervention iv = dlg.getIntervention();
        if (Database::instance().addIntervention(iv)) {
            refreshPatientDetail();
            m_detailTabs->setCurrentIndex(1);
            statusBar()->showMessage("Intervencija uspjesno dodana.", 3000);
        }
    }
}

void MainWindow::onEditIntervention(int id)
{
    Intervention iv = Database::instance().getIntervention(id);
    InterventionDialog dlg(this, m_currentPatient.id, iv);
    if (dlg.exec() == QDialog::Accepted) {
        if (Database::instance().updateIntervention(dlg.getIntervention())) {
            refreshPatientDetail();
            statusBar()->showMessage("Intervencija azurirana.", 3000);
        }
    }
}

void MainWindow::onDeleteIntervention(int id)
{
    if (Database::instance().deleteIntervention(id)) {
        refreshPatientDetail();
        statusBar()->showMessage("Intervencija obrisana.", 3000);
    }
}

void MainWindow::onViewPdf(const QString& path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

// ── Print slots ───────────────────────────────────────────────────────────────
void MainWindow::onPrintPatientReport()
{
    if (m_currentPatient.id == 0) {
        QMessageBox::information(this, "Napomena", "Odaberite pacijenta iz liste.");
        return;
    }
    auto ivs = Database::instance().getPatientInterventions(m_currentPatient.id);

    // Provjeri ima li priloženih PDF-ova
    bool hasAnyPdf     = false;
    bool hasAnyConsent = false;
    bool hasAnyHealth  = false;
    bool hasAnyPhotos  = false;
    for (const auto& iv : ivs) {
        if (!iv.pdfPath.isEmpty())       hasAnyPdf     = true;
        if (!iv.consentPdfPath.isEmpty()) hasAnyConsent = true;
        if (!iv.healthPdfPath.isEmpty())  hasAnyHealth  = true;
        if (!iv.photoBefore.isEmpty() || !iv.photoAfter.isEmpty()) hasAnyPhotos = true;
    }

    bool printPdf     = false;
    bool printConsent = false;
    bool printHealth  = false;
    bool printPhotos  = false;
    bool hidePrice    = false;

    // Uvijek prikaži dijalog sa opcijama
    {
        QDialog dlg(this);
        dlg.setWindowTitle("Opcije štampe");
        dlg.setModal(true);
        dlg.setFixedWidth(380);

        auto* lay = new QVBoxLayout(&dlg);
        lay->setSpacing(8);
        lay->setContentsMargins(20,20,20,20);

        auto* titleLbl = new QLabel("🖨 Opcije štampe");
        titleLbl->setStyleSheet("font-size:13px;font-weight:bold;color:#1565C0;");
        lay->addWidget(titleLbl);

        auto* sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("background:#E0E0E0;");
        lay->addWidget(sep);

        auto makeChk = [&](const QString& txt, const QString& sub,
                            const QString& col, bool checked=false) -> QCheckBox* {
            auto* chk = new QCheckBox;
            chk->setChecked(checked);
            auto* row = new QHBoxLayout;
            auto* label = new QLabel(txt);
            label->setStyleSheet("font-weight:bold;color:#212121;font-size:11px;");
            auto* subLbl = new QLabel(sub);
            subLbl->setStyleSheet("color:#757575;font-size:9px;");
            auto* col1 = new QVBoxLayout;
            col1->addWidget(label);
            col1->addWidget(subLbl);
            row->addWidget(chk);
            row->addLayout(col1, 1);
            auto* w = new QWidget;
            w->setLayout(row);
            w->setStyleSheet(
                QString("QWidget{background:%1;border-radius:6px;padding:5px;}").arg(col));
            lay->addWidget(w);
            return chk;
        };

        // Karton — uvijek
        auto* chkKarton = makeChk("🦷 Karton pacijenta",
            "Kompletan medicinski karton sa svim intervencijama", "#F3F9FD", true);
        chkKarton->setEnabled(false);

        // Fotografije
        QCheckBox* chkPhotos = nullptr;
        if (hasAnyPhotos)
            chkPhotos = makeChk("📷 Fotografije prije/poslije",
                "Prikaži fotografije zuba u stampi", "#FFF8E1");

        // PDF dokumenti
        QCheckBox* chkPdf = nullptr;
        if (hasAnyPdf)
            chkPdf = makeChk("📄 Priloženi PDF dokumenti",
                "Dokumenti priloženi uz intervencije", "#F1F8E9");

        // Upitnik o zdravlju
        QCheckBox* chkHealth = nullptr;
        if (hasAnyHealth)
            chkHealth = makeChk("🏥 Upitnici o zdravstvenom stanju",
                "PDF upitnici o zdravlju pacijenta", "#E3F2FD");

        // Pristanak
        QCheckBox* chkConsent = nullptr;
        if (hasAnyConsent)
            chkConsent = makeChk("📝 Pristanci na intervenciju",
                "PDF pristanci na intervenciju", "#F9F0FF");

        // Separator
        auto* sep2 = new QFrame;
        sep2->setFrameShape(QFrame::HLine);
        sep2->setStyleSheet("background:#E0E0E0;");
        lay->addWidget(sep2);

        // Sakrij cijenu
        auto* chkHidePrice = makeChk("🚫 Sakrij cijenu",
            "Cijena se neće prikazivati u stampi", "#FFF3E0");

        lay->addSpacing(4);

        auto* btnRow = new QHBoxLayout;
        auto* cancelBtn = new QPushButton("Odustani");
        auto* printBtn  = new QPushButton("🖨 Štampaj");
        cancelBtn->setStyleSheet(
            "QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
            "background:white;color:#424242;padding:6px 16px;font-weight:bold;}"
            "QPushButton:hover{background:#F5F5F5;}");
        printBtn->setStyleSheet(
            "QPushButton{background:#E65100;color:white;border-radius:4px;"
            "border:none;padding:6px 20px;font-weight:bold;}"
            "QPushButton:hover{background:#EF6C00;}");
        btnRow->addStretch();
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(printBtn);
        lay->addLayout(btnRow);

        connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(printBtn,  &QPushButton::clicked, &dlg, &QDialog::accept);

        if (dlg.exec() != QDialog::Accepted) return;

        if (chkPhotos  && chkPhotos->isChecked())  printPhotos  = true;
        if (chkPdf     && chkPdf->isChecked())      printPdf     = true;
        if (chkHealth  && chkHealth->isChecked())   printHealth  = true;
        if (chkConsent && chkConsent->isChecked())  printConsent = true;
        hidePrice = chkHidePrice->isChecked();
    }

    // Štampaj
    ReportGenerator::printPatientReport(m_currentPatient, ivs, this,
                                        printPdf, printConsent,
                                        printHealth, printPhotos, hidePrice);
}

void MainWindow::onPrintIntervention() {}

// ── Search slots ──────────────────────────────────────────────────────────────
void MainWindow::onSearch()
{
    QString name = m_searchName->text().trimmed();
    QDate from   = m_searchFrom->date();
    QDate to     = m_searchTo->date();
    QString type = m_searchType->currentIndex() == 0
                   ? QString() : m_searchType->currentText();

    m_searchIvs = Database::instance().searchInterventions(name, from, to, type);
    m_searchResults->setRowCount(m_searchIvs.size());

    for (int i = 0; i < m_searchIvs.size(); ++i) {
        const auto& iv = m_searchIvs[i];
        Patient p = Database::instance().getPatient(iv.patientId);
        QString dt = iv.dateTime.length() >= 10 ?
            QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd")
                .toString("dd.MM.yyyy.") : iv.dateTime;
        QString sl = iv.status=="completed" ? "Zavrseno" :
                     iv.status=="scheduled" ? "Zakazano" : "Otkazano";
        m_searchResults->setItem(i, 0, tItem(p.fullName()));
        m_searchResults->setItem(i, 1, tItem(dt, true));
        m_searchResults->setItem(i, 2, tItem(iv.interventionType));
        m_searchResults->setItem(i, 3, tItem(iv.tooth, true));
        m_searchResults->setItem(i, 4, tItem(iv.doctor));
        m_searchResults->setItem(i, 5, tItem(
            QString::number(iv.cost,'f',2)+" "+iv.currency, true));
        m_searchResults->setItem(i, 6, tItem(sl, true));
    }
    statusBar()->showMessage(
        QString("Pronadjeno: %1 rezultata").arg(m_searchIvs.size()));
}

void MainWindow::onSearchResultDoubleClicked(int row, int)
{
    if (row < 0 || row >= m_searchIvs.size()) return;
    m_currentPatient = Database::instance().getPatient(m_searchIvs[row].patientId);
    refreshPatientDetail();
    loadPatientList();
    selectPatientById(m_currentPatient.id);
    m_rightStack->setCurrentIndex(1);
}

// ── Appointment slots ─────────────────────────────────────────────────────────
void MainWindow::onAddAppointment()
{
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Zakazi Termin");
    dlg->setMinimumWidth(440);
    dlg->setModal(true);

    auto* lay = new QVBoxLayout(dlg);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(12);

    auto* title = new QLabel("Zakazivanje termina");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#1565C0;");
    lay->addWidget(title);

    const QString fs =
        "QComboBox,QLineEdit,QDateTimeEdit,QTextEdit{"
        "border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;min-height:30px;}"
        "QComboBox:focus,QLineEdit:focus,QDateTimeEdit:focus,QTextEdit:focus{"
        "border:2px solid #00695C;}";

    auto* form = new QFormLayout;
    auto makeLbl = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("font-weight:bold;color:#424242;");
        return l;
    };

    auto* patCombo = new QComboBox;
    patCombo->setStyleSheet(fs);
    auto allP = Database::instance().getAllPatients();
    for (const auto& p : allP)
        patCombo->addItem(p.fullName(), p.id);
    if (m_currentPatient.id > 0) {
        for (int i = 0; i < patCombo->count(); ++i)
            if (patCombo->itemData(i).toInt() == m_currentPatient.id) {
                patCombo->setCurrentIndex(i); break;
            }
    }

    auto* dtEdit = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1));
    dtEdit->setCalendarPopup(true);
    dtEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    dtEdit->setStyleSheet(fs);

    auto* reasonEdit = new QLineEdit;
    reasonEdit->setPlaceholderText("Razlog dolaska...");
    reasonEdit->setStyleSheet(fs);

    auto* notesEdit = new QTextEdit;
    notesEdit->setPlaceholderText("Napomene...");
    notesEdit->setFixedHeight(70);
    notesEdit->setStyleSheet(fs);

    form->addRow(makeLbl("Pacijent:"),      patCombo);
    form->addRow(makeLbl("Datum/Vrijeme:"), dtEdit);
    form->addRow(makeLbl("Razlog:"),        reasonEdit);
    form->addRow(makeLbl("Napomene:"),      notesEdit);
    lay->addLayout(form);

    auto* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto* cancelB = new QPushButton("Odustani");
    cancelB->setStyleSheet(
        "QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
        "background:white;color:#424242;padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#F5F5F5;}");
    auto* saveB = makeActionBtn("Zakazi", "#00695C", "#00897B");
    btnRow->addWidget(cancelB);
    btnRow->addWidget(saveB);
    lay->addLayout(btnRow);

    connect(cancelB, &QPushButton::clicked, dlg, &QDialog::reject);
    connect(saveB, &QPushButton::clicked, [&](){
        Appointment ap;
        ap.patientId = patCombo->currentData().toInt();
        ap.dateTime  = dtEdit->dateTime().toString("yyyy-MM-dd HH:mm");
        ap.reason    = reasonEdit->text().trimmed();
        ap.status    = "scheduled";
        ap.notes     = notesEdit->toPlainText().trimmed();
        Database::instance().addAppointment(ap);
        dlg->accept();
    });

    if (dlg->exec() == QDialog::Accepted) {
        refreshDashboard();
        refreshAppointments();
        statusBar()->showMessage("Termin zakazan.", 3000);
    }
    dlg->deleteLater();
}

// ── Clinic Settings ───────────────────────────────────────────────────────────
void MainWindow::onOpenClinicSettings()
{
    ClinicSettingsDialog dlg(this);
    connect(&dlg, &ClinicSettingsDialog::settingsChanged,
            this, &MainWindow::applyClinicBranding);
    dlg.exec();
}

void MainWindow::applyClinicBranding()
{
    auto& cs = ClinicSettings::instance();

    // Update sidebar name labels
    m_clinicNameLbl->setText(cs.clinicName);
    m_clinicSubLbl->setText(cs.clinicSubtitle);

    // Update window title
    setWindowTitle(cs.clinicName.isEmpty() ? "DentaPro" : cs.clinicName);

    // Update dashboard welcome text
    if (m_welcomeLbl) {
        m_welcomeLbl->setText(cs.clinicName.isEmpty()
            ? "Dobrodosli!"
            : "Dobrodosli u " + cs.clinicName);
    }

    // Show logo if available
    if (cs.hasLogo()) {
        QPixmap px = cs.logo();
        if (!px.isNull()) {
            m_logoImageLbl->setPixmap(
                px.scaled(240, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_logoImageLbl->show();
            m_logoImageLbl->setToolTip(cs.clinicName);
        }
    } else {
        m_logoImageLbl->hide();
        m_logoImageLbl->setPixmap(QPixmap());
    }
}

// ── Statistics tab ────────────────────────────────────────────────────────────
void MainWindow::setupStatisticsTab()
{
    m_statsWidget = new StatisticsWidget;
    m_rightStack->addWidget(m_statsWidget); // index 4
}

// ── Export slots ──────────────────────────────────────────────────────────────
void MainWindow::onExportPatientCsv()
{
    if (m_currentPatient.id == 0) {
        QMessageBox::information(this, "Napomena", "Odaberite pacijenta iz liste.");
        return;
    }
    auto ivs = Database::instance().getPatientInterventions(m_currentPatient.id);
    ExportManager::exportPatientToCsv(m_currentPatient, ivs, this);
}

void MainWindow::onExportFullCsv()
{
    ExportManager::exportFullReportToCsv(this);
}

void MainWindow::onExportPatientXlsx()
{
    if (m_currentPatient.id == 0) {
        QMessageBox::information(this, "Napomena", "Odaberite pacijenta iz liste.");
        return;
    }
    auto ivs = Database::instance().getPatientInterventions(m_currentPatient.id);
    ExportManager::exportPatientToXlsx(m_currentPatient, ivs, this);
}

void MainWindow::onExportFullXlsx()
{
    ExportManager::exportFullReportToXlsx(this);
}

void MainWindow::onExportPatientPdf()
{
    if (m_currentPatient.id == 0) {
        QMessageBox::information(this, "Napomena", "Odaberite pacijenta iz liste.");
        return;
    }
    auto ivs = Database::instance().getPatientInterventions(m_currentPatient.id);
    ExportManager::exportPatientToPdf(m_currentPatient, ivs, this);
}

void MainWindow::onExportFullPdf()
{
    ExportManager::exportFullReportToPdf(this);
}

// ── Backup slot ───────────────────────────────────────────────────────────────
void MainWindow::onBackupDatabase()
{
    ExportManager::backupDatabase(this);
}

// ── Refresh appointments table ────────────────────────────────────────────────
void MainWindow::refreshAppointments(const QString& statusFilter)
{
    // Ucitaj termine — sve buducih 365 dana + prosli 30 dana
    QSqlQuery q(QSqlDatabase::database());
    QString sql =
        "SELECT a.*, p.firstName, p.lastName FROM appointments a "
        "JOIN patients p ON a.patientId = p.id "
        "WHERE date(a.dateTime) >= date('now','-30 days') ";
    if (!statusFilter.isEmpty())
        sql += "AND a.status = '" + statusFilter + "' ";
    sql += "ORDER BY a.dateTime ASC";
    q.exec(sql);

    m_appointTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        m_appointTable->insertRow(row);

        QString patName = q.value("firstName").toString() + " "
                        + q.value("lastName").toString();
        QString dt = q.value("dateTime").toString();
        if (dt.length() >= 16)
            dt = QDateTime::fromString(dt, "yyyy-MM-dd HH:mm")
                     .toString("dd.MM.yyyy  HH:mm");

        QString status  = q.value("status").toString();
        QString statusTxt = status == "scheduled"  ? "Zakazano"  :
                            status == "completed"   ? "Zavrseno"  :
                            status == "cancelled"   ? "Otkazano"  : "No-show";
        QColor statusColor = status == "scheduled"  ? QColor("#E8F5E9") :
                             status == "completed"   ? QColor("#E3F2FD") :
                             status == "cancelled"   ? QColor("#FFEBEE") :
                                                       QColor("#FFF8E1");
        QColor statusText  = status == "scheduled"  ? QColor("#2E7D32") :
                             status == "completed"   ? QColor("#1565C0") :
                             status == "cancelled"   ? QColor("#C62828") :
                                                       QColor("#E65100");

        auto mkItem = [](const QString& t, bool center = false) {
            auto* it = new QTableWidgetItem(t);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            if (center) it->setTextAlignment(Qt::AlignCenter);
            return it;
        };

        m_appointTable->setItem(row, 0, mkItem(patName));
        m_appointTable->setItem(row, 1, mkItem(dt, true));
        m_appointTable->setItem(row, 2, mkItem(q.value("reason").toString()));

        auto* statusItem = mkItem(statusTxt, true);
        statusItem->setBackground(statusColor);
        statusItem->setForeground(statusText);
        QFont sf; sf.setBold(true); statusItem->setFont(sf);
        m_appointTable->setItem(row, 3, statusItem);
        m_appointTable->setItem(row, 4, mkItem(q.value("notes").toString()));

        // Dugme za brisanje
        int apId = q.value("id").toInt();
        auto* delBtn = new QPushButton("Obrisi");
        delBtn->setStyleSheet(
            "QPushButton{background:#EF5350;color:white;border:none;"
            "border-radius:3px;padding:3px 8px;font-size:10px;}"
            "QPushButton:hover{background:#E53935;}");
        connect(delBtn, &QPushButton::clicked, [this, apId](){
            auto ans = QMessageBox::question(this, "Brisanje",
                "Obrisati ovaj termin?",
                QMessageBox::Yes | QMessageBox::No);
            if (ans == QMessageBox::Yes) {
                Database::instance().deleteAppointment(apId);
                refreshAppointments();
                refreshDashboard();
            }
        });
        m_appointTable->setCellWidget(row, 5, delBtn);

        // Istakni ako je danas
        if (q.value("dateTime").toString().startsWith(
                QDate::currentDate().toString("yyyy-MM-dd"))) {
            for (int c = 0; c < 5; ++c)
                if (m_appointTable->item(row, c))
                    m_appointTable->item(row, c)->setBackground(QColor("#FFF9C4"));
        }
        ++row;
    }

    if (row == 0) {
        m_appointTable->insertRow(0);
        auto* noItem = new QTableWidgetItem("Nema termina za odabrani filter.");
        noItem->setTextAlignment(Qt::AlignCenter);
        noItem->setForeground(QColor("#9E9E9E"));
        m_appointTable->setItem(0, 0, noItem);
        m_appointTable->setSpan(0, 0, 1, 6);
    }
}
