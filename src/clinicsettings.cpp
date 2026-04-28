#include "clinicsettings.h"
#include <QSettings>
#include <QPixmap>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

ClinicSettings& ClinicSettings::instance()
{
    static ClinicSettings s;
    return s;
}

// Vraca putanju do settings.ini — UVIJEK pored .exe fajla
static QString iniPath()
{
    return QCoreApplication::applicationDirPath() + "/settings.ini";
}

void ClinicSettings::load()
{
    // Prvo provjeri da li postoji settings.ini pored .exe
    QString path = iniPath();

    // Ako ne postoji, provjeri da li ima starih podataka u Registry
    // i migruj ih u ini fajl
    if (!QFile::exists(path)) {
        QSettings reg("DentaPro", "DentaPro");
        reg.beginGroup("Clinic");
        if (reg.contains("name")) {
            // Migruj iz Registry u INI
            QSettings ini(path, QSettings::IniFormat);
            ini.beginGroup("Clinic");
            ini.setValue("name",          reg.value("name"));
            ini.setValue("subtitle",      reg.value("subtitle"));
            ini.setValue("address",       reg.value("address"));
            ini.setValue("phone",         reg.value("phone"));
            ini.setValue("email",         reg.value("email"));
            ini.setValue("logoPath",      reg.value("logoPath"));
            ini.setValue("doctors",       reg.value("doctors"));
            ini.setValue("defaultDoctor", reg.value("defaultDoctor"));
            ini.endGroup();
        }
        reg.endGroup();
    }

    QSettings s(path, QSettings::IniFormat);
    s.beginGroup("Clinic");
    clinicName     = s.value("name",          "DentaPro").toString();
    clinicSubtitle = s.value("subtitle",      "Stomatoloska ordinacija").toString();
    clinicAddress  = s.value("address",       "").toString();
    clinicPhone    = s.value("phone",         "").toString();
    clinicEmail    = s.value("email",         "").toString();
    logoPath       = s.value("logoPath",      "").toString();
    doctors        = s.value("doctors",       QStringList()).toStringList();
    defaultDoctor  = s.value("defaultDoctor", "").toString();
    if (doctors.isEmpty()) doctors << "Dr.";
    customInterventionTypes = s.value("customInterventionTypes", QStringList()).toStringList();
    s.endGroup();
}

void ClinicSettings::save()
{
    QSettings s(iniPath(), QSettings::IniFormat);
    s.beginGroup("Clinic");
    s.setValue("name",          clinicName);
    s.setValue("subtitle",      clinicSubtitle);
    s.setValue("address",       clinicAddress);
    s.setValue("phone",         clinicPhone);
    s.setValue("email",         clinicEmail);
    s.setValue("logoPath",      logoPath);
    s.setValue("doctors",                 doctors);
    s.setValue("defaultDoctor",           defaultDoctor);
    s.setValue("customInterventionTypes", customInterventionTypes);
    s.endGroup();
    s.sync();
}

QPixmap ClinicSettings::logo() const
{
    if (logoPath.isEmpty() || !QFile::exists(logoPath))
        return {};
    return QPixmap(logoPath);
}

bool ClinicSettings::hasLogo() const
{
    return !logoPath.isEmpty() && QFile::exists(logoPath);
}
