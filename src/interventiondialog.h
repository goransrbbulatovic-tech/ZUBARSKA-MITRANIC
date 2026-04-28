#pragma once
#include <QDialog>
#include "models.h"

class QLineEdit;
class QComboBox;
class QDateTimeEdit;
class QTextEdit;
class QLabel;
class QDoubleSpinBox;
class QTabWidget;
class SignatureWidget;

class InterventionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InterventionDialog(QWidget* parent = nullptr,
                                 int patientId = 0,
                                 const Intervention& iv = {});
    Intervention getIntervention() const;

private slots:
    void accept() override;
    void browsePdf();
    void clearSignature();
    void browsePhotoBefore();
    void browsePhotoAfter();
    void browseHealthPdf();
    void browseConsentPdf();
    void clearConsentSignature();
    void onTypeChanged(const QString& text);

private:
    void setupUi();
    void populateFields();
    void updatePhotoPreview(QLabel* lbl, const QString& path);

    Intervention     m_iv;
    int              m_patientId = 0;

    // Tab 1 - Osnovno
    QComboBox*       m_type             = nullptr;
    QLineEdit*       m_tooth            = nullptr;
    QComboBox*       m_doctorCombo      = nullptr;
    QDateTimeEdit*   m_dateTime         = nullptr;
    QComboBox*       m_status           = nullptr;
    QLineEdit*       m_product          = nullptr;

    // Tab 2 - Medicinski podaci
    QTextEdit*       m_diagnosis        = nullptr;
    QTextEdit*       m_treatment        = nullptr;
    QTextEdit*       m_description      = nullptr;
    QTextEdit*       m_notes            = nullptr;

    // Tab 3 - Finansije + PDF + Potpis
    QDoubleSpinBox*  m_cost             = nullptr;
    QComboBox*       m_currency         = nullptr;
    QLineEdit*       m_pdfPath          = nullptr;
    SignatureWidget* m_signature        = nullptr;
    QString          m_savedSigPath;

    // Tab 4 - Fotografije
    QLabel*          m_photoBeforePrev  = nullptr;
    QLabel*          m_photoAfterPrev   = nullptr;
    QString          m_photoBeforePath;
    QString          m_photoAfterPath;

    // Tab 5 - Upitnik o zdravstvenom stanju
    QLineEdit*       m_healthPdfPath    = nullptr;

    // Tab 6 - Pristanak na intervenciju
    SignatureWidget* m_consentSignature = nullptr;
    QLineEdit*       m_consentPdfPath   = nullptr;
    QString          m_savedConsentSigPath;
};
