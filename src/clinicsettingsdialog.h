#pragma once
#include <QDialog>
class QLineEdit;
class QLabel;
class QPushButton;
class QListWidget;

class ClinicSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ClinicSettingsDialog(QWidget* parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void browseLogo();
    void clearLogo();
    void accept() override;

private:
    void setupUi();
    void loadCurrentValues();
    void updateLogoPreview();

    QLineEdit*   m_name        = nullptr;
    QLineEdit*   m_subtitle    = nullptr;
    QLineEdit*   m_address     = nullptr;
    QLineEdit*   m_phone       = nullptr;
    QLineEdit*   m_email       = nullptr;
    QLabel*      m_logoPreview = nullptr;
    QLabel*      m_logoPath    = nullptr;
    QPushButton* m_clearBtn    = nullptr;
    QListWidget* m_doctorList  = nullptr;
    QLineEdit*   m_doctorInput = nullptr;

    QString      m_pendingLogoPath;
};
