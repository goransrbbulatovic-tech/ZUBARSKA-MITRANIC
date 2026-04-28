#pragma once
#include <QWidget>
#include <QList>
#include <QDate>
#include "models.h"

class QCalendarWidget;
class QListWidget;
class QLabel;
class QTextBrowser;

class PatientHistoryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PatientHistoryWidget(QWidget* parent = nullptr);

    void setPatient(const Patient& patient);
    void refresh();

signals:
    void editIntervention(int interventionId);
    void deleteIntervention(int interventionId);
    void viewPdf(const QString& path);

private slots:
    void onDateSelected(const QDate& date);
    void onInterventionClicked(int row);
    void onEditClicked();
    void onDeleteClicked();
    void onViewPdfClicked();
    void onViewConsentClicked();
    void onViewPristanakClicked();

private:
    void loadInterventionDates();
    void loadInterventionsForDate(const QDate& date);
    QString buildInterventionHtml(const Intervention& iv);

    Patient          m_patient;
    QList<QDate>     m_markedDates;
    QList<Intervention> m_currentIvs;

    QCalendarWidget* m_calendar   = nullptr;
    QListWidget*     m_ivList     = nullptr;
    QTextBrowser*    m_detail     = nullptr;
    QLabel*          m_dateLbl    = nullptr;
};
