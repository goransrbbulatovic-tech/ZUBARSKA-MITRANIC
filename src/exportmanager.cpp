#include "exportmanager.h"
#include "database.h"
#include "clinicsettings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

// ── CSV helpers ───────────────────────────────────────────────────────────────
static QString csvEscape(const QString& s)
{
    QString r = s;
    r.replace("\"", "\"\"");
    if (r.contains(',') || r.contains('"') || r.contains('\n'))
        r = "\"" + r + "\"";
    return r;
}

static void writeRow(QTextStream& ts, const QStringList& cols)
{
    QStringList esc;
    for (auto& c : cols) esc << csvEscape(c);
    ts << esc.join(",") << "\n";
}

// ── Patient CSV ───────────────────────────────────────────────────────────────
bool ExportManager::exportPatientToCsv(const Patient& p,
                                         const QList<Intervention>& ivs,
                                         QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/" + p.fullName().replace(" ", "_") + "_intervencije.csv";

    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi u Excel/CSV", def,
        "CSV fajlovi (*.csv);;Svi fajlovi (*.*)");
    if (path.isEmpty()) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Greska",
            "Nije moguce kreirati fajl:\n" + path);
        return false;
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);

    // BOM for Excel UTF-8
    ts << "\xEF\xBB\xBF";

    // Clinic header
    auto& cs = ClinicSettings::instance();
    ts << "# " << cs.clinicName << "\n";
    ts << "# Izvoz: " << QDateTime::currentDateTime()
                              .toString("dd.MM.yyyy HH:mm") << "\n";
    ts << "#\n";

    // Patient info
    ts << "# PACIJENT\n";
    writeRow(ts, {"Ime i prezime", p.fullName()});
    writeRow(ts, {"Datum rodjenja", p.dateOfBirth});
    writeRow(ts, {"Telefon", p.phone});
    writeRow(ts, {"Email", p.email});
    writeRow(ts, {"Alergije", p.allergies});
    writeRow(ts, {"Krvna grupa", p.bloodType});
    ts << "#\n";

    // Interventions header
    writeRow(ts, {"Datum", "Vrsta intervencije", "Zub", "Dijagnoza",
                  "Tretman", "Doktor", "Cijena", "Valuta", "Status", "Napomene"});

    double ukupno = 0;
    for (const auto& iv : ivs) {
        QString datum = iv.dateTime.length() >= 10
            ? QDate::fromString(iv.dateTime.left(10), "yyyy-MM-dd")
                  .toString("dd.MM.yyyy.")
            : iv.dateTime;
        QString status = iv.status == "completed" ? "Zavrseno" :
                         iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
        writeRow(ts, {datum, iv.interventionType, iv.tooth,
                      iv.diagnosis, iv.treatment, iv.doctor,
                      QString::number(iv.cost, 'f', 2),
                      iv.currency, status, iv.notes});
        if (iv.status == "completed") ukupno += iv.cost;
    }

    ts << "#\n";
    writeRow(ts, {"UKUPNO ZAVRSENO", "", "", "", "", "",
                  QString::number(ukupno, 'f', 2), "BAM", "", ""});

    f.close();

    QMessageBox::information(parent, "Izvoz uspjesan",
        QString("Fajl je sacuvan:\n%1\n\nOtvori ga u Microsoft Excelu ili Google Sheets.")
            .arg(path));
    return true;
}

// ── Full clinic CSV ───────────────────────────────────────────────────────────
bool ExportManager::exportFullReportToCsv(QWidget* parent)
{
    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/DentaPro_izvjestaj_"
                  + QDate::currentDate().toString("yyyy-MM-dd") + ".csv";

    QString path = QFileDialog::getSaveFileName(
        parent, "Izvezi kompletan izvjestaj", def,
        "CSV fajlovi (*.csv);;Svi fajlovi (*.*)");
    if (path.isEmpty()) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, "Greska",
            "Nije moguce kreirati fajl.");
        return false;
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    ts << "\xEF\xBB\xBF";

    auto& cs = ClinicSettings::instance();
    ts << "# " << cs.clinicName << " - Kompletan izvjestaj\n";
    ts << "# Datum: " << QDate::currentDate().toString("dd.MM.yyyy.") << "\n#\n";

    writeRow(ts, {"Pacijent", "Datum rodjenja", "Telefon",
                  "Datum intervencije", "Vrsta", "Zub",
                  "Doktor", "Cijena (BAM)", "Status"});

    auto patients = Database::instance().getAllPatients();
    double grandTotal = 0;
    for (const auto& p : patients) {
        auto ivs = Database::instance().getPatientInterventions(p.id);
        for (const auto& iv : ivs) {
            QString datum = iv.dateTime.length() >= 10
                ? QDate::fromString(iv.dateTime.left(10), "yyyy-MM-dd")
                      .toString("dd.MM.yyyy.")
                : iv.dateTime;
            QString status = iv.status == "completed" ? "Zavrseno" :
                             iv.status == "scheduled"  ? "Zakazano"  : "Otkazano";
            writeRow(ts, {p.fullName(), p.dateOfBirth, p.phone,
                          datum, iv.interventionType, iv.tooth,
                          iv.doctor,
                          QString::number(iv.cost, 'f', 2), status});
            if (iv.status == "completed") grandTotal += iv.cost;
        }
    }

    ts << "#\n";
    writeRow(ts, {"UKUPNI PRIHOD", "", "", "", "", "", "",
                  QString::number(grandTotal, 'f', 2), ""});
    f.close();

    QMessageBox::information(parent, "Izvoz uspjesan",
        QString("Kompletan izvjestaj sacuvan:\n%1").arg(path));
    return true;
}

// ── Database backup ───────────────────────────────────────────────────────────
bool ExportManager::backupDatabase(QWidget* parent)
{
    QString dbPath = QStandardPaths::writableLocation(
                         QStandardPaths::AppDataLocation)
                     + "/dentapro.db";

    QString def = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                  + "/DentaPro_backup_"
                  + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm")
                  + ".db";

    QString dest = QFileDialog::getSaveFileName(
        parent, "Sacuvaj backup baze podataka", def,
        "Baza podataka (*.db);;Svi fajlovi (*.*)");
    if (dest.isEmpty()) return false;

    if (QFile::exists(dest)) QFile::remove(dest);

    if (QFile::copy(dbPath, dest)) {
        QMessageBox::information(parent, "Backup uspjesan",
            QString("Backup sacuvan:\n%1\n\n"
                    "Cuvajte ovaj fajl na sigurnom mjestu (USB, cloud).").arg(dest));
        return true;
    } else {
        QMessageBox::critical(parent, "Greska",
            "Backup nije uspio. Provjerite dozvole i slobodan prostor.");
        return false;
    }
}

// ── Auto-backup ───────────────────────────────────────────────────────────────
void ExportManager::autoBackup()
{
    QString dbPath = QStandardPaths::writableLocation(
                         QStandardPaths::AppDataLocation)
                     + "/dentapro.db";
    if (!QFile::exists(dbPath)) return;

    QString backupDir = QStandardPaths::writableLocation(
                            QStandardPaths::AppDataLocation)
                        + "/backups";
    QDir().mkpath(backupDir);

    // Keep last 7 daily backups
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QString dest  = backupDir + "/dentapro_" + today + ".db";

    if (!QFile::exists(dest)) {
        QFile::copy(dbPath, dest);

        // Delete backups older than 7 days
        QDir d(backupDir);
        auto entries = d.entryList({"dentapro_*.db"}, QDir::Files,
                                    QDir::Name);
        while (entries.size() > 7) {
            d.remove(entries.first());
            entries.removeFirst();
        }
    }
}
