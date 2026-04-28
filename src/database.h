#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QList>
#include <QPair>
#include <QDate>
#include <QDebug>
#include "models.h"

class Database
{
public:
    static Database& instance();

    bool initialize(const QString& dbPath = QString());

    // ── Patients ─────────────────────────────────────────────────────────────
    bool           addPatient(Patient& patient);
    bool           updatePatient(const Patient& patient);
    bool           deletePatient(int id);
    QList<Patient> getAllPatients();                          // backwards compat
    QList<Patient> searchPatients(const QString& query);     // backwards compat
    Patient        getPatient(int id);

    // Paginated — koristi ovo za velike liste
    QList<Patient> getPatientsPage(int offset, int limit,
                                   const QString& search = QString());
    int            countPatients(const QString& search = QString());

    // ── Interventions ─────────────────────────────────────────────────────────
    bool                addIntervention(Intervention& iv);
    bool                updateIntervention(const Intervention& iv);
    bool                deleteIntervention(int id);
    QList<Intervention> getPatientInterventions(int patientId); // backwards compat
    QList<Intervention> getPatientInterventionsPage(int patientId,
                                                    int offset, int limit);
    int                 countPatientInterventions(int patientId);
    QList<Intervention> getInterventionsByDate(int patientId, const QDate& date);
    QList<QDate>        getInterventionDates(int patientId);
    Intervention        getIntervention(int id);

    // ── Search ────────────────────────────────────────────────────────────────
    QList<Intervention> searchInterventions(const QString& patientName,
                                            const QDate& from,
                                            const QDate& to,
                                            const QString& type = QString());
    // Paginated search
    QList<Intervention> searchInterventionsPage(const QString& patientName,
                                                const QDate& from,
                                                const QDate& to,
                                                const QString& type,
                                                int offset, int limit);
    int countSearchInterventions(const QString& patientName,
                                 const QDate& from,
                                 const QDate& to,
                                 const QString& type = QString());

    // ── Appointments ──────────────────────────────────────────────────────────
    bool               addAppointment(Appointment& ap);
    bool               updateAppointment(const Appointment& ap);
    bool               deleteAppointment(int id);
    QList<Appointment> getUpcomingAppointments(int days = 30);
    QList<Appointment> getPatientAppointments(int patientId);

    // ── Stats ─────────────────────────────────────────────────────────────────
    int    totalPatients();
    int    totalInterventionsThisMonth();
    double totalRevenueThisMonth();
    QList<QPair<QString,double>> revenueByMonth(int year);
    QList<QPair<QString,int>>    interventionsByType(int year);

    // ── Reminders ─────────────────────────────────────────────────────────────
    QList<Appointment> getTodayAppointments();
    QList<Appointment> getAppointmentsInDays(int days);

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool         createTables();
    Patient      patientFromQuery(QSqlQuery& q);
    Intervention interventionFromQuery(QSqlQuery& q);
    Appointment  appointmentFromQuery(QSqlQuery& q);

    QSqlDatabase m_db;
};
