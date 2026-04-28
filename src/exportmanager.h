#pragma once
#include "models.h"
#include <QList>
#include <QString>
#include <QWidget>

class ExportManager
{
public:
    // Export all interventions for a patient to CSV (opens in Excel)
    static bool exportPatientToCsv(const Patient& p,
                                    const QList<Intervention>& ivs,
                                    QWidget* parent = nullptr);

    // Export full clinic report (all patients + interventions) to CSV
    static bool exportFullReportToCsv(QWidget* parent = nullptr);

    // Backup database file
    static bool backupDatabase(QWidget* parent = nullptr);

    // Auto-backup to AppData/DentaPro/backups/ (called on startup)
    static void autoBackup();
};
