#include "clinicsettingsdialog.h"
#include "clinicsettings.h"
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QScrollArea>
#include <QListWidget>
#include <QFrame>
#include <QCoreApplication>

ClinicSettingsDialog::ClinicSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Podesavanja ordinacije");
    setMinimumWidth(540);
    setMinimumHeight(620);
    setModal(true);
    setupUi();
    loadCurrentValues();
}

void ClinicSettingsDialog::setupUi()
{
    auto* mainLay = new QVBoxLayout(this);
    mainLay->setSpacing(0);
    mainLay->setContentsMargins(0, 0, 0, 0);

    // ── Header ────────────────────────────────────────────────────────────────
    auto* hdr = new QWidget;
    hdr->setStyleSheet("background:#1565C0;padding:16px;");
    auto* hLay = new QHBoxLayout(hdr);
    hLay->setContentsMargins(16, 12, 16, 12);
    auto* htitle = new QLabel("Podesavanja ordinacije");
    htitle->setStyleSheet("color:white;font-size:18px;font-weight:bold;");
    auto* hsub = new QLabel("Logo, naziv i kontakt informacije");
    hsub->setStyleSheet("color:rgba(255,255,255,0.75);font-size:10px;");
    auto* hl = new QVBoxLayout;
    hl->addWidget(htitle); hl->addWidget(hsub);
    hLay->addLayout(hl);
    mainLay->addWidget(hdr);

    // ── Scrollable body ───────────────────────────────────────────────────────
    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    auto* body = new QWidget;
    body->setStyleSheet("background:#FAFAFA;");
    auto* bLay = new QVBoxLayout(body);
    bLay->setContentsMargins(24, 20, 24, 16);
    bLay->setSpacing(20);

    const QString fieldStyle =
        "QLineEdit{border:1px solid #BDBDBD;border-radius:4px;"
        "padding:6px 10px;min-height:32px;background:white;}"
        "QLineEdit:focus{border:2px solid #1565C0;}";
    const QString lblStyle = "font-weight:bold;color:#424242;font-size:11px;";

    // ── Logo section ──────────────────────────────────────────────────────────
    auto* logoGroup = new QGroupBox("Logo ordinacije");
    logoGroup->setStyleSheet(
        "QGroupBox{font-weight:bold;color:#1565C0;border:1px solid #BBDEFB;"
        "border-radius:6px;margin-top:8px;padding-top:8px;}"
        "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px;}");
    auto* logoLay = new QVBoxLayout(logoGroup);
    logoLay->setSpacing(10);

    // Preview area
    auto* previewBox = new QWidget;
    previewBox->setFixedHeight(130);
    previewBox->setStyleSheet(
        "background:white;border:2px dashed #90CAF9;border-radius:8px;");
    auto* pvLay = new QHBoxLayout(previewBox);
    pvLay->setContentsMargins(16, 8, 16, 8);

    m_logoPreview = new QLabel;
    m_logoPreview->setAlignment(Qt::AlignCenter);
    m_logoPreview->setFixedSize(200, 110);
    m_logoPreview->setStyleSheet("border:none;");
    m_logoPreview->setText("Nema loga\n(kliknite Ucitaj logo)");
    m_logoPreview->setStyleSheet("color:#9E9E9E;font-size:11px;border:none;");

    auto* pvRight = new QVBoxLayout;
    pvRight->setSpacing(6);
    m_logoPath = new QLabel("Nije odabrana slika");
    m_logoPath->setStyleSheet("color:#757575;font-size:10px;word-wrap:true;");
    m_logoPath->setWordWrap(true);

    auto* browseBtn = new QPushButton("Ucitaj logo...");
    browseBtn->setStyleSheet(
        "QPushButton{background:#1565C0;color:white;border-radius:4px;"
        "border:none;padding:8px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#1976D2;}");

    m_clearBtn = new QPushButton("Ukloni logo");
    m_clearBtn->setStyleSheet(
        "QPushButton{background:#EF5350;color:white;border-radius:4px;"
        "border:none;padding:6px 14px;font-weight:bold;}"
        "QPushButton:hover{background:#E53935;}");
    m_clearBtn->setEnabled(false);

    auto* infoLbl = new QLabel(
        "Podrzani formati: PNG, JPG, BMP\n"
        "Preporucena velicina: 300x100 px ili veca");
    infoLbl->setStyleSheet("color:#9E9E9E;font-size:9px;");

    pvRight->addWidget(m_logoPath);
    pvRight->addWidget(browseBtn);
    pvRight->addWidget(m_clearBtn);
    pvRight->addWidget(infoLbl);
    pvRight->addStretch();

    pvLay->addWidget(m_logoPreview);
    pvLay->addLayout(pvRight, 1);
    logoLay->addWidget(previewBox);
    bLay->addWidget(logoGroup);

    // ── Clinic info section ───────────────────────────────────────────────────
    auto* infoGroup = new QGroupBox("Informacije o ordinaciji");
    infoGroup->setStyleSheet(logoGroup->styleSheet());
    auto* infoForm = new QFormLayout(infoGroup);
    infoForm->setSpacing(10);
    infoForm->setLabelAlignment(Qt::AlignRight);

    auto makeLbl = [&](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet(lblStyle);
        return l;
    };

    m_name = new QLineEdit;
    m_name->setPlaceholderText("npr. Ordinacija Dr. Petrovic");
    m_name->setStyleSheet(fieldStyle);

    m_subtitle = new QLineEdit;
    m_subtitle->setPlaceholderText("npr. Stomatoloska ordinacija");
    m_subtitle->setStyleSheet(fieldStyle);

    m_address = new QLineEdit;
    m_address->setPlaceholderText("Ulica i broj, Grad");
    m_address->setStyleSheet(fieldStyle);

    m_phone = new QLineEdit;
    m_phone->setPlaceholderText("+387 xx xxx-xxx");
    m_phone->setStyleSheet(fieldStyle);

    m_email = new QLineEdit;
    m_email->setPlaceholderText("ordinacija@email.ba");
    m_email->setStyleSheet(fieldStyle);

    infoForm->addRow(makeLbl("Naziv ordinacije*:"), m_name);
    infoForm->addRow(makeLbl("Podnaslov:"),         m_subtitle);
    infoForm->addRow(makeLbl("Adresa:"),            m_address);
    infoForm->addRow(makeLbl("Telefon:"),           m_phone);
    infoForm->addRow(makeLbl("E-mail:"),            m_email);
    bLay->addWidget(infoGroup);

    // ── Doctors section ───────────────────────────────────────────────────────
    auto* docGroup = new QGroupBox("Lista doktora");
    docGroup->setStyleSheet(logoGroup->styleSheet());
    auto* docLay = new QVBoxLayout(docGroup);

    auto* docHint = new QLabel("Doktori ce biti dostupni u padajucem meniju pri unosu intervencije.");
    docHint->setStyleSheet("color:#757575;font-size:10px;");
    docHint->setWordWrap(true);
    docLay->addWidget(docHint);

    m_doctorList = new QListWidget;
    m_doctorList->setFixedHeight(100);
    m_doctorList->setStyleSheet(
        "QListWidget{border:1px solid #BDBDBD;border-radius:4px;background:white;}"
        "QListWidget::item{padding:4px 8px;}"
        "QListWidget::item:selected{background:#E3F2FD;color:#1565C0;}");
    docLay->addWidget(m_doctorList);

    auto* docInputRow = new QHBoxLayout;
    m_doctorInput = new QLineEdit;
    m_doctorInput->setPlaceholderText("Ime doktora (npr. Dr. Petrovic)");
    m_doctorInput->setStyleSheet(fieldStyle);
    auto* addDocBtn = new QPushButton("Dodaj");
    addDocBtn->setStyleSheet(
        "QPushButton{background:#00695C;color:white;border-radius:4px;"
        "border:none;padding:6px 14px;font-weight:bold;}"
        "QPushButton:hover{background:#00897B;}");
    auto* delDocBtn = new QPushButton("Ukloni");
    delDocBtn->setStyleSheet(
        "QPushButton{background:#EF5350;color:white;border-radius:4px;"
        "border:none;padding:6px 14px;font-weight:bold;}"
        "QPushButton:hover{background:#E53935;}");
    docInputRow->addWidget(m_doctorInput, 1);
    docInputRow->addWidget(addDocBtn);
    docInputRow->addWidget(delDocBtn);
    docLay->addLayout(docInputRow);
    bLay->addWidget(docGroup);

    connect(addDocBtn, &QPushButton::clicked, [this](){
        QString name = m_doctorInput->text().trimmed();
        if (name.isEmpty()) return;
        m_doctorList->addItem(name);
        m_doctorInput->clear();
    });
    connect(delDocBtn, &QPushButton::clicked, [this](){
        delete m_doctorList->takeItem(m_doctorList->currentRow());
    });

    // Preview note
    auto* note = new QLabel(
        "Naziv i logo se prikazuju u zaglavlju programa i na svim stampanjima.");
    note->setStyleSheet(
        "background:#E8F5E9;border:1px solid #C8E6C9;border-radius:4px;"
        "padding:8px 12px;color:#2E7D32;font-size:10px;");
    note->setWordWrap(true);
    bLay->addWidget(note);
    bLay->addStretch();

    scroll->setWidget(body);
    mainLay->addWidget(scroll, 1);

    // ── Buttons bar ───────────────────────────────────────────────────────────
    auto* btnBar = new QWidget;
    btnBar->setStyleSheet("background:#EEEEEE;border-top:1px solid #E0E0E0;");
    auto* btnLay = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(24, 12, 24, 12);
    btnLay->addStretch();

    auto* cancelBtn = new QPushButton("Odustani");
    cancelBtn->setMinimumSize(100, 36);
    cancelBtn->setStyleSheet(
        "QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
        "background:white;color:#424242;font-weight:bold;}"
        "QPushButton:hover{background:#F5F5F5;}");

    auto* saveBtn = new QPushButton("Sacuvaj podesavanja");
    saveBtn->setMinimumSize(170, 36);
    saveBtn->setStyleSheet(
        "QPushButton{background:#1565C0;color:white;border-radius:4px;"
        "border:none;font-weight:bold;}"
        "QPushButton:hover{background:#1976D2;}");

    btnLay->addWidget(cancelBtn);
    btnLay->addWidget(saveBtn);
    mainLay->addWidget(btnBar);

    connect(browseBtn, &QPushButton::clicked, this, &ClinicSettingsDialog::browseLogo);
    connect(m_clearBtn, &QPushButton::clicked, this, &ClinicSettingsDialog::clearLogo);
    connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn,    &QPushButton::clicked, this, &ClinicSettingsDialog::accept);
}

void ClinicSettingsDialog::loadCurrentValues()
{
    auto& cs = ClinicSettings::instance();
    m_name->setText(cs.clinicName);
    m_subtitle->setText(cs.clinicSubtitle);
    m_address->setText(cs.clinicAddress);
    m_phone->setText(cs.clinicPhone);
    m_email->setText(cs.clinicEmail);
    m_pendingLogoPath = cs.logoPath;
    updateLogoPreview();

    // Load doctors
    m_doctorList->clear();
    for (const auto& d : cs.doctors)
        m_doctorList->addItem(d);
}

void ClinicSettingsDialog::browseLogo()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Odaberite logo ordinacije",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Slike (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)");

    if (path.isEmpty()) return;

    // Try loading to validate
    QPixmap px(path);
    if (px.isNull()) {
        QMessageBox::warning(this, "Greska",
            "Odabrana slika nije valjana.\nPodrzani formati: PNG, JPG, BMP");
        return;
    }

    m_pendingLogoPath = path;
    updateLogoPreview();
}

void ClinicSettingsDialog::clearLogo()
{
    m_pendingLogoPath.clear();
    updateLogoPreview();
}

void ClinicSettingsDialog::updateLogoPreview()
{
    if (m_pendingLogoPath.isEmpty()) {
        m_logoPreview->setPixmap(QPixmap());
        m_logoPreview->setText("Nema loga\n(kliknite Ucitaj logo)");
        m_logoPreview->setStyleSheet("color:#9E9E9E;font-size:11px;border:none;");
        m_logoPath->setText("Nije odabrana slika");
        m_clearBtn->setEnabled(false);
        return;
    }

    QPixmap px(m_pendingLogoPath);
    if (!px.isNull()) {
        m_logoPreview->setPixmap(
            px.scaled(200, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_logoPreview->setStyleSheet("border:none;");
        m_logoPreview->setText("");
    }

    // Show just filename
    QFileInfo fi(m_pendingLogoPath);
    m_logoPath->setText(fi.fileName() + "\n(" +
        QString::number(px.width()) + "x" +
        QString::number(px.height()) + " px)");
    m_clearBtn->setEnabled(true);
}

void ClinicSettingsDialog::accept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Greska", "Naziv ordinacije je obavezan.");
        m_name->setFocus();
        return;
    }

    auto& cs       = ClinicSettings::instance();
    cs.clinicName     = m_name->text().trimmed();
    cs.clinicSubtitle = m_subtitle->text().trimmed();
    cs.clinicAddress  = m_address->text().trimmed();
    cs.clinicPhone    = m_phone->text().trimmed();
    cs.clinicEmail    = m_email->text().trimmed();

    // Copy logo next to exe so it travels with the program
    if (!m_pendingLogoPath.isEmpty()) {
        QString exeDir = QCoreApplication::applicationDirPath();
        QFileInfo fi(m_pendingLogoPath);
        QString destPath = exeDir + "/logo." + fi.suffix().toLower();
        // If source is already in exe folder, just use it
        if (QFileInfo(m_pendingLogoPath).absolutePath() != QFileInfo(exeDir).absolutePath()) {
            if (QFile::exists(destPath)) QFile::remove(destPath);
            QFile::copy(m_pendingLogoPath, destPath);
        }
        cs.logoPath = destPath;
    } else {
        cs.logoPath = "";
    }

    cs.save();
    // Save doctors
    QStringList docs;
    for (int i = 0; i < m_doctorList->count(); ++i)
        docs << m_doctorList->item(i)->text();
    cs.doctors = docs;
    cs.save();
    emit settingsChanged();
    QDialog::accept();
}
