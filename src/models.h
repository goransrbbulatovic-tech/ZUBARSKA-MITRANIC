#pragma once
#include <QString>
#include <QDateTime>
#include <QDate>

// ── Patient ──────────────────────────────────────────────────────────────────
struct Patient {
    int     id          = 0;
    QString firstName;
    QString lastName;
    QString dateOfBirth;
    QString gender;
    QString phone;
    QString email;
    QString address;
    QString allergies;
    QString bloodType;
    QString nationalId;   // Matični broj (JMBG)
    QString notes;
    QString createdAt;

    QString fullName() const { return firstName + " " + lastName; }
};

// ── Intervention ─────────────────────────────────────────────────────────────
struct Intervention {
    int     id               = 0;
    int     patientId        = 0;
    QString interventionType;
    QString tooth;
    QString diagnosis;
    QString treatment;
    QString description;
    QString doctor;
    QString product;           // proizvod/materijal kojim je radjeno
    QString dateTime;          // ISO: "yyyy-MM-dd HH:mm"
    QString pdfPath;
    QString signatureImagePath;
    QString photoBefore;       // path to "before" photo
    QString photoAfter;        // path to "after" photo
    QString consentSignaturePath; // potpis pristanka pacijenta
    QString healthPdfPath;        // upitnik o zdravstvenom stanju (PDF)
    QString consentPdfPath;       // pristanak na intervenciju (PDF)
    double  cost             = 0.0;
    QString currency         = "BAM";
    QString status;            // completed | scheduled | cancelled
    QString notes;
};

// ── Appointment ──────────────────────────────────────────────────────────────
struct Appointment {
    int     id        = 0;
    int     patientId = 0;
    QString dateTime;
    QString reason;
    QString status;   // scheduled | completed | cancelled | no-show
    QString notes;
};
