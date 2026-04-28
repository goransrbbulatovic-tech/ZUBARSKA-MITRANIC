#pragma once
#include <QString>
#include <QStringList>
#include <QPixmap>

class ClinicSettings
{
public:
    static ClinicSettings& instance();
    void load();
    void save();

    // Branding
    QString     clinicName;
    QString     clinicSubtitle;
    QString     clinicAddress;
    QString     clinicPhone;
    QString     clinicEmail;
    QString     logoPath;

    // Doctors
    QStringList doctors;
    QString     defaultDoctor;

    // Custom intervention types (korisnik dodaje svoje)
    QStringList customInterventionTypes;

    QPixmap logo() const;
    bool    hasLogo() const;

private:
    ClinicSettings() = default;
};
