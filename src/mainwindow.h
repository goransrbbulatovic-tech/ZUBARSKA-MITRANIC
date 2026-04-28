#pragma once
#include <QMainWindow>
#include "models.h"

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QStackedWidget;
class QLabel;
class QTabWidget;
class QTableWidget;
class QDateEdit;
class QComboBox;
class PatientHistoryWidget;
class StatisticsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onAddPatient();
    void onEditPatient();
    void onDeletePatient();
    void onPatientSelected(QListWidgetItem* item);
    void onSearchPatients(const QString& text);
    void onAddIntervention();
    void onEditIntervention(int id);
    void onDeleteIntervention(int id);
    void onViewPdf(const QString& path);
    void onPrintPatientReport();
    void onPrintIntervention();
    void onSearch();
    void onSearchResultDoubleClicked(int row, int col);
    void refreshDashboard();
    void onAddAppointment();
    void onOpenClinicSettings();
    void applyClinicBranding();
    void onExportPatientCsv();
    void onExportFullCsv();
    void onExportPatientXlsx();
    void onExportFullXlsx();
    void onExportPatientPdf();
    void onExportFullPdf();
    void onBackupDatabase();
    void onLoadMorePatients();
    void onLoadMoreInterventions();
    void refreshAppointments(const QString& statusFilter = "scheduled");

private:
    void setupUi();
    void setupMenu();
    void setupToolbar();
    QWidget* buildLeftPanel();
    QWidget* buildRightPanel();
    void setupDashboard();
    void setupPatientsTab();
    void setupSearchTab();
    void setupAppointmentsTab();
    void setupStatisticsTab();

    void loadPatientList();
    void selectPatientById(int id);
    void refreshPatientDetail();
    void showPatientCard(const Patient& p);

    // UI refs
    QListWidget*          m_patientList        = nullptr;
    QLineEdit*            m_searchBox          = nullptr;
    QStackedWidget*       m_rightStack         = nullptr;

    // Clinic branding (sidebar)
    QWidget*              m_logoWidget         = nullptr;
    QLabel*               m_logoImageLbl       = nullptr;
    QLabel*               m_clinicNameLbl      = nullptr;
    QLabel*               m_clinicSubLbl       = nullptr;

    QWidget*              m_dashWidget         = nullptr;
    QLabel*               m_statPatients       = nullptr;
    QLabel*               m_statIvMonth        = nullptr;
    QLabel*               m_statRevMonth       = nullptr;
    QTableWidget*         m_upcomingTable      = nullptr;
    QLabel*               m_welcomeLbl         = nullptr;
    QTableWidget*         m_birthdayTable      = nullptr;

    QWidget*              m_detailWidget       = nullptr;
    QTabWidget*           m_detailTabs         = nullptr;
    QLabel*               m_patientNameLbl     = nullptr;
    QLabel*               m_patientInfoLbl     = nullptr;
    QTableWidget*         m_allIvTable         = nullptr;
    PatientHistoryWidget* m_historyWidget      = nullptr;

    QWidget*              m_searchWidget       = nullptr;
    QLineEdit*            m_searchName         = nullptr;
    QDateEdit*            m_searchFrom         = nullptr;
    QDateEdit*            m_searchTo           = nullptr;
    QComboBox*            m_searchType         = nullptr;
    QTableWidget*         m_searchResults      = nullptr;
    QList<Intervention>   m_searchIvs;

    QWidget*              m_appointmentsWidget = nullptr;
    QTableWidget*         m_appointTable       = nullptr;

    StatisticsWidget*     m_statsWidget        = nullptr;

    Patient               m_currentPatient;

    // ── Paginacija pacijenata ────────────────────────────────────────────────
    static const int      PAGE_SIZE = 100;   // pacijenata po stranici
    int                   m_patientOffset = 0;
    int                   m_patientTotal  = 0;
    QString               m_lastSearch;
    QLabel*               m_patientCountLbl = nullptr;
    QPushButton*          m_loadMoreBtn     = nullptr;

    // ── Paginacija intervencija ──────────────────────────────────────────────
    static const int      IV_PAGE_SIZE = 25; // intervencija po stranici
    int                   m_ivOffset   = 0;
    int                   m_ivTotal    = 0;
    QPushButton*          m_ivLoadMoreBtn = nullptr;
    QLabel*               m_ivCountLbl    = nullptr;
};
