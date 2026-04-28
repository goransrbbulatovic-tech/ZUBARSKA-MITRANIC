#pragma once
#include <QDialog>
#include "models.h"

class QLineEdit;
class QComboBox;
class QDateEdit;
class QTextEdit;
class QLabel;

class PatientDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PatientDialog(QWidget* parent = nullptr, const Patient& patient = {});
    Patient getPatient() const;

private slots:
    void accept() override;

private:
    void setupUi();
    void populateFields();

    Patient     m_patient;
    QLineEdit*  m_firstName  = nullptr;
    QLineEdit*  m_lastName   = nullptr;
    QDateEdit*  m_dob        = nullptr;
    QComboBox*  m_gender     = nullptr;
    QLineEdit*  m_phone      = nullptr;
    QLineEdit*  m_email      = nullptr;
    QLineEdit*  m_address    = nullptr;
    QLineEdit*  m_allergies  = nullptr;
    QLineEdit*  m_nationalId = nullptr;
    QComboBox*  m_bloodType  = nullptr;
    QTextEdit*  m_notes      = nullptr;
};
