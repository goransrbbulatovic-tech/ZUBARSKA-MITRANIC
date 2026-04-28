#include "patientdialog.h"
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QScrollArea>

PatientDialog::PatientDialog(QWidget* parent, const Patient& patient)
    : QDialog(parent), m_patient(patient)
{
    setWindowTitle(patient.id == 0 ? "Novi Pacijent" : "Uredi Pacijenta");
    setMinimumWidth(520);
    setModal(true);
    setupUi();
    if (patient.id != 0) populateFields();
}

void PatientDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    // ── Header ───────────────────────────────────────────────────────────────
    auto* header = new QWidget;
    header->setStyleSheet("background:#1565C0; padding:16px;");
    auto* hLay = new QHBoxLayout(header);
    auto* titleLbl = new QLabel(m_patient.id == 0 ? "➕  Novi Pacijent" : "✏️  Uredi Pacijenta");
    titleLbl->setStyleSheet("color:white; font-size:18px; font-weight:bold;");
    hLay->addWidget(titleLbl);
    mainLayout->addWidget(header);

    // ── Scrollable form ──────────────────────────────────────────────────────
    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    auto* formWidget = new QWidget;
    formWidget->setStyleSheet("background:#FAFAFA;");
    auto* form = new QFormLayout(formWidget);
    form->setSpacing(10);
    form->setContentsMargins(24,24,24,12);
    form->setLabelAlignment(Qt::AlignRight);

    auto makeEdit = [](const QString& placeholder) {
        auto* e = new QLineEdit;
        e->setPlaceholderText(placeholder);
        e->setMinimumHeight(34);
        e->setStyleSheet("QLineEdit{border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;}"
                         "QLineEdit:focus{border:2px solid #1565C0;}");
        return e;
    };

    m_firstName = makeEdit("Unesite ime");
    m_lastName  = makeEdit("Unesite prezime");

    m_dob = new QDateEdit;
    m_dob->setCalendarPopup(true);
    m_dob->setDate(QDate(1990,1,1));
    m_dob->setMinimumHeight(34);
    m_dob->setStyleSheet("QDateEdit{border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;}"
                         "QDateEdit:focus{border:2px solid #1565C0;}");

    m_gender = new QComboBox;
    m_gender->addItems({"Muški","Ženski","Ostalo"});
    m_gender->setMinimumHeight(34);
    m_gender->setStyleSheet("QComboBox{border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;}"
                            "QComboBox:focus{border:2px solid #1565C0;}");

    m_phone      = makeEdit("+387 xx xxx-xxx");
    m_email      = makeEdit("email@primjer.ba");
    m_address    = makeEdit("Ulica, Grad, Država");
    m_allergies  = makeEdit("Penicillin, latex...");
    m_nationalId = makeEdit("npr. 1234567890123 (JMBG)");

    m_bloodType = new QComboBox;
    m_bloodType->addItems({"Nepoznata","A+","A-","B+","B-","AB+","AB-","O+","O-"});
    m_bloodType->setMinimumHeight(34);
    m_bloodType->setStyleSheet(m_gender->styleSheet());

    m_notes = new QTextEdit;
    m_notes->setPlaceholderText("Napomene o pacijentu...");
    m_notes->setMinimumHeight(80);
    m_notes->setMaximumHeight(120);
    m_notes->setStyleSheet("QTextEdit{border:1px solid #BDBDBD;border-radius:4px;padding:4px;}"
                           "QTextEdit:focus{border:2px solid #1565C0;}");

    auto makeLbl = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet("font-weight:600; color:#424242;");
        return l;
    };

    form->addRow(makeLbl("Ime*:"), m_firstName);
    form->addRow(makeLbl("Prezime*:"), m_lastName);
    form->addRow(makeLbl("Datum rođenja:"), m_dob);
    form->addRow(makeLbl("pol:"), m_gender);
    form->addRow(makeLbl("Telefon:"), m_phone);
    form->addRow(makeLbl("E-mail:"), m_email);
    form->addRow(makeLbl("Adresa:"), m_address);
    form->addRow(makeLbl("Alergije:"),      m_allergies);
    form->addRow(makeLbl("Matični broj:"), m_nationalId);
    form->addRow(makeLbl("Krvna grupa:"),   m_bloodType);
    form->addRow(makeLbl("Napomene:"), m_notes);

    scroll->setWidget(formWidget);
    mainLayout->addWidget(scroll, 1);

    // ── Buttons ───────────────────────────────────────────────────────────────
    auto* btnBar = new QWidget;
    btnBar->setStyleSheet("background:#EEEEEE; border-top:1px solid #E0E0E0;");
    auto* btnLay = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(24,12,24,12);
    btnLay->addStretch();

    auto* cancelBtn = new QPushButton("Odustani");
    cancelBtn->setMinimumSize(100,36);
    cancelBtn->setStyleSheet("QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
                             "background:white;color:#424242;font-weight:bold;}"
                             "QPushButton:hover{background:#F5F5F5;}");

    auto* saveBtn = new QPushButton(m_patient.id == 0 ? "Dodaj Pacijenta" : "Sačuvaj");
    saveBtn->setMinimumSize(130,36);
    saveBtn->setStyleSheet("QPushButton{background:#1565C0;color:white;border-radius:4px;"
                           "border:none;font-weight:bold;}"
                           "QPushButton:hover{background:#1976D2;}");

    btnLay->addWidget(cancelBtn);
    btnLay->addWidget(saveBtn);
    mainLayout->addWidget(btnBar);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn,   &QPushButton::clicked, this, &PatientDialog::accept);
}

void PatientDialog::populateFields()
{
    m_firstName->setText(m_patient.firstName);
    m_lastName->setText(m_patient.lastName);
    if (!m_patient.dateOfBirth.isEmpty())
        m_dob->setDate(QDate::fromString(m_patient.dateOfBirth,"yyyy-MM-dd"));
    int gi = m_gender->findText(m_patient.gender);
    if (gi >= 0) m_gender->setCurrentIndex(gi);
    m_phone->setText(m_patient.phone);
    m_email->setText(m_patient.email);
    m_address->setText(m_patient.address);
    m_allergies->setText(m_patient.allergies);
    m_nationalId->setText(m_patient.nationalId);
    int bi = m_bloodType->findText(m_patient.bloodType);
    if (bi >= 0) m_bloodType->setCurrentIndex(bi);
    m_notes->setPlainText(m_patient.notes);
}

void PatientDialog::accept()
{
    if (m_firstName->text().trimmed().isEmpty() ||
        m_lastName->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Greška", "Ime i prezime su obavezni.");
        return;
    }
    m_patient.firstName  = m_firstName->text().trimmed();
    m_patient.lastName   = m_lastName->text().trimmed();
    m_patient.dateOfBirth= m_dob->date().toString("yyyy-MM-dd");
    m_patient.gender     = m_gender->currentText();
    m_patient.phone      = m_phone->text().trimmed();
    m_patient.email      = m_email->text().trimmed();
    m_patient.address    = m_address->text().trimmed();
    m_patient.allergies  = m_allergies->text().trimmed();
    m_patient.nationalId = m_nationalId->text().trimmed();
    m_patient.bloodType  = m_bloodType->currentText();
    m_patient.notes      = m_notes->toPlainText().trimmed();
    QDialog::accept();
}

Patient PatientDialog::getPatient() const { return m_patient; }
