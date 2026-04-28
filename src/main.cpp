#include <QApplication>
#include <QMessageBox>
#include <QSplashScreen>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QSqlDatabase>
#include <QSystemTrayIcon>
#include <QFile>
#include "mainwindow.h"
#include "database.h"
#include "clinicsettings.h"
#include "exportmanager.h"

static QSplashScreen* createSplash()
{
    auto& cs = ClinicSettings::instance(); // vec ucitan u main()

    const int W = 560, H = 340;
    QPixmap pix(W, H);
    pix.fill(QColor("#1565C0"));

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 20));
    p.drawRoundedRect(16, 16, W-32, H-32, 14, 14);

    // Decorative circles
    p.setBrush(QColor(255, 255, 255, 10));
    p.drawEllipse(W-130, H-130, 220, 220);
    p.setBrush(QColor(255, 255, 255, 8));
    p.drawEllipse(W-90, H-90, 150, 150);

    int contentY = 40;

    // ── Logo image ───────────────────────────────────────────────────────────
    if (cs.hasLogo()) {
        QPixmap logo = cs.logo();
        if (!logo.isNull()) {
            QPixmap scaled = logo.scaled(210, 80,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int lx = (W - scaled.width()) / 2;
            p.drawPixmap(lx, contentY, scaled);
            contentY += scaled.height() + 14;
        }
    }

    // ── Clinic name — auto font, word wrap ───────────────────────────────────
    QString name = cs.clinicName.isEmpty() ? "DentaPro" : cs.clinicName;

    int fontSize = 28;
    if      (name.length() > 30) fontSize = 17;
    else if (name.length() > 22) fontSize = 20;
    else if (name.length() > 15) fontSize = 24;

    p.setPen(Qt::white);
    QFont fName("Segoe UI", fontSize, QFont::Bold);
    p.setFont(fName);
    QRect nameRect(30, contentY, W-60, (fontSize+8)*3);
    p.drawText(nameRect, Qt::AlignCenter | Qt::TextWordWrap, name);
    QFontMetrics fmName(fName);
    QRect nameBound = fmName.boundingRect(nameRect,
                        Qt::AlignCenter | Qt::TextWordWrap, name);
    contentY += qMax(nameBound.height(), fontSize + 10) + 12;

    // ── Subtitle ─────────────────────────────────────────────────────────────
    if (contentY < H - 80) {
        QString sub = cs.clinicSubtitle.isEmpty()
                      ? "Stomatoloska ordinacija"
                      : cs.clinicSubtitle;
        p.setPen(QColor(255, 255, 255, 210));
        QFont fSub("Segoe UI", 12);
        p.setFont(fSub);
        p.drawText(QRect(30, contentY, W-60, 28),
                   Qt::AlignCenter | Qt::TextWordWrap, sub);
        contentY += 34;
    }

    // ── Dobrodošli ────────────────────────────────────────────────────────────
    if (contentY < H - 70) {
        p.setPen(QColor(255, 255, 255, 180));
        QFont fWelcome("Segoe UI", 10);
        p.setFont(fWelcome);
        p.drawText(QRect(30, contentY, W-60, 22),
                   Qt::AlignCenter, "Dobrodosli!");
        contentY += 26;
    }

    // ── Address + phone ───────────────────────────────────────────────────────
    QStringList contactParts;
    if (!cs.clinicAddress.isEmpty()) contactParts << cs.clinicAddress;
    if (!cs.clinicPhone.isEmpty())   contactParts << cs.clinicPhone;
    if (!contactParts.isEmpty() && contentY < H - 50) {
        p.setPen(QColor(255, 255, 255, 160));
        QFont fContact("Segoe UI", 9);
        p.setFont(fContact);
        QString contactStr = contactParts.join("   |   ");
        p.drawText(QRect(30, contentY, W-60, 40),
                   Qt::AlignCenter | Qt::TextWordWrap, contactStr);
    }

    // ── Loading text + progress bar ───────────────────────────────────────────
    p.setPen(QColor(255, 255, 255, 120));
    QFont fLoad("Segoe UI", 9);
    p.setFont(fLoad);
    p.drawText(QRect(0, H-36, W, 20), Qt::AlignCenter, "Ucitavanje...");

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 40));
    p.drawRoundedRect(40, H-14, W-80, 6, 3, 3);
    p.setBrush(QColor(255, 255, 255, 180));
    p.drawRoundedRect(40, H-14, (W-80)/2, 6, 3, 3);

    return new QSplashScreen(pix);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Load settings first — treba za naziv programa i splash
    ClinicSettings::instance().load();
    auto& cs = ClinicSettings::instance();
    QString appName = cs.clinicName.isEmpty() ? "DentaPro" : cs.clinicName;

    app.setApplicationName(appName);
    app.setApplicationVersion("1.0");
    app.setOrganizationName(appName);

    // Create splash
    QSplashScreen* splash = createSplash();
    splash->show();
    app.processEvents();

    // Check SQLite driver
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        splash->hide();
        QMessageBox::critical(nullptr, "Greska - SQLite nedostaje",
            "SQLite driver nije pronadjen!\n\n"
            "Provjerite da folder 'sqldrivers' postoji pored DentaPro.exe\n"
            "i da sadrzi 'qsqlite.dll'.");
        return 1;
    }

    // Init database
    if (!Database::instance().initialize()) {
        splash->hide();
        QMessageBox::critical(nullptr, "Greska baze podataka",
            "Nije moguce pokrenuti bazu podataka.\n"
            "Provjerite dozvole za pisanje u AppData folder.");
        return 1;
    }

    // Auto-backup database
    ExportManager::autoBackup();

    // System tray for reminders
    QSystemTrayIcon* tray = nullptr;
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        tray = new QSystemTrayIcon(&app);
        tray->setToolTip(cs.clinicName.isEmpty() ? "DentaPro" : cs.clinicName);
        tray->show();
    }

    MainWindow* window = nullptr;
    QTimer::singleShot(1600, [&](){
        window = new MainWindow();
        window->show();
        splash->finish(window);
        splash->deleteLater();

        // Show today's appointment reminder
        if (tray) {
            auto todayApts = Database::instance().getTodayAppointments();
            if (!todayApts.isEmpty()) {
                tray->showMessage(
                    cs.clinicName + " - Termini danas",
                    QString("Imate %1 termin(a) zakazano danas.")
                        .arg(todayApts.size()),
                    QSystemTrayIcon::Information, 5000);
            }
        }
    });

    return app.exec();
}
