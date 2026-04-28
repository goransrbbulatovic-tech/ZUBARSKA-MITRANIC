#pragma once
#include <QString>
#include <QStringList>
#include "models.h"

class QPrinter;
class QWidget;

class ReportGenerator
{
public:
    static void printPatientReport(const Patient& patient,
                                   const QList<Intervention>& interventions,
                                   QWidget* parent      = nullptr,
                                   bool includePdfs     = false,
                                   bool includeConsent  = false,
                                   bool includeHealth   = false,
                                   bool includePhotos   = true,
                                   bool hidePrice       = false);

    static void printIntervention(const Intervention& iv,
                                  const Patient& patient,
                                  QWidget* parent = nullptr);

    static QString buildPatientReportHtml(const Patient& patient,
                                          const QList<Intervention>& interventions,
                                          bool includePdfs    = false,
                                          bool includeConsent = false,
                                          bool includeHealth  = false,
                                          bool includePhotos  = true,
                                          bool hidePrice      = false);

    static QString buildInterventionHtml(const Intervention& iv,
                                         const Patient& patient);

    static QString pdfToHtmlImages(const QString& pdfPath, int maxWidthPx = 700);
};
