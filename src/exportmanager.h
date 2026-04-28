#pragma once
#include "models.h"
#include <QList>
#include <QString>
#include <QWidget>

class ExportManager
{
public:
    // ── CSV (Excel) ───────────────────────────────────────────────────────────
    static bool exportPatientToCsv(const Patient& p,
                                    const QList<Intervention>& ivs,
                                    QWidget* parent = nullptr);

    static bool exportFullReportToCsv(QWidget* parent = nullptr);

    // ── Excel XLSX ────────────────────────────────────────────────────────────
    static bool exportPatientToXlsx(const Patient& p,
                                     const QList<Intervention>& ivs,
                                     QWidget* parent = nullptr);

    static bool exportFullReportToXlsx(QWidget* parent = nullptr);

    // ── PDF ───────────────────────────────────────────────────────────────────
    static bool exportPatientToPdf(const Patient& p,
                                    const QList<Intervention>& ivs,
                                    QWidget* parent = nullptr);

    static bool exportFullReportToPdf(QWidget* parent = nullptr);

    // ── Backup ────────────────────────────────────────────────────────────────
    static bool backupDatabase(QWidget* parent = nullptr);
    static void autoBackup();

private:
    static QString buildExportHtml(const Patient& p,
                                    const QList<Intervention>& ivs,
                                    bool singlePatient = true);
    static QString buildFullReportHtml(
        const QList<Patient>& patients,
        const QMap<int, QList<Intervention>>& ivMap);
};
