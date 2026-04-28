#include "statisticswidget.h"
#include "database.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QDate>
#include <QtMath>
#include <QFrame>

static const QList<QColor> CHART_COLORS = {
    QColor("#1565C0"), QColor("#00695C"), QColor("#E65100"),
    QColor("#6A1B9A"), QColor("#AD1457"), QColor("#0277BD"),
    QColor("#2E7D32"), QColor("#F57F17"), QColor("#4527A0"),
    QColor("#00838F")
};

StatisticsWidget::StatisticsWidget(QWidget* parent) : QWidget(parent)
{
    setupUi();
    refresh();
}

void StatisticsWidget::setupUi()
{
    setStyleSheet("background:#F5F5F5;");
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 20, 24, 20);
    lay->setSpacing(16);

    // Header
    auto* hRow = new QHBoxLayout;
    auto* title = new QLabel("Statistike i Grafikoni");
    title->setStyleSheet("font-size:22px;font-weight:bold;color:#1565C0;");
    hRow->addWidget(title, 1);

    m_yearCombo = new QComboBox;
    m_yearCombo->setStyleSheet(
        "QComboBox{border:1px solid #BDBDBD;border-radius:4px;"
        "padding:4px 10px;min-height:30px;background:white;}");
    int cy = QDate::currentDate().year();
    for (int y = cy; y >= cy - 4; --y)
        m_yearCombo->addItem(QString::number(y), y);
    hRow->addWidget(new QLabel("Godina:"));
    hRow->addWidget(m_yearCombo);

    auto* refreshBtn = new QPushButton("Osvjezi");
    refreshBtn->setStyleSheet(
        "QPushButton{background:#1565C0;color:white;border-radius:4px;"
        "border:none;padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#1976D2;}");
    hRow->addWidget(refreshBtn);
    lay->addLayout(hRow);

    // Summary cards row
    auto* cardsRow = new QHBoxLayout;
    cardsRow->setSpacing(12);

    auto makeCard = [&](const QString& label, QLabel*& val,
                        const QString& bg) -> QWidget* {
        auto* card = new QWidget;
        card->setStyleSheet(
            QString("QWidget{background:%1;border-radius:8px;}").arg(bg));
        card->setMinimumHeight(80);
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(16, 12, 16, 12);
        val = new QLabel("...");
        val->setStyleSheet("font-size:22px;font-weight:bold;color:white;");
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("font-size:10px;color:rgba(255,255,255,0.8);");
        cl->addWidget(val);
        cl->addWidget(lbl);
        return card;
    };

    cardsRow->addWidget(makeCard("Ukupno pacijenata",      m_lblTotalPat, "#1565C0"));
    cardsRow->addWidget(makeCard("Ukupno intervencija",    m_lblTotalIv,  "#00695C"));
    cardsRow->addWidget(makeCard("Ukupni prihod (BAM)",    m_lblTotalRev, "#E65100"));
    cardsRow->addWidget(makeCard("Prosjek po intervenciji",m_lblAvgRev,   "#6A1B9A"));
    lay->addLayout(cardsRow);

    // Charts area — scrollable
    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);

    auto* chartsWidget = new QWidget;
    chartsWidget->setStyleSheet("background:#F5F5F5;");
    auto* chartsLay = new QVBoxLayout(chartsWidget);
    chartsLay->setContentsMargins(0, 0, 0, 0);
    chartsLay->setSpacing(16);

    // Painted chart area (this widget)
    // We'll use a separate inner widget for painting
    auto* paintArea = new QWidget;
    paintArea->setMinimumHeight(520);
    paintArea->setStyleSheet("background:#F5F5F5;");
    paintArea->installEventFilter(this);
    chartsLay->addWidget(paintArea);
    chartsLay->addStretch();
    scroll->setWidget(chartsWidget);
    lay->addWidget(scroll, 1);

    connect(refreshBtn, &QPushButton::clicked, this, &StatisticsWidget::refresh);
    connect(m_yearCombo, &QComboBox::currentIndexChanged,
            this, &StatisticsWidget::refresh);
}

void StatisticsWidget::refresh()
{
    loadData();

    // Update summary cards
    double total = 0;
    for (auto& p : m_monthlyRevenue) total += p.second;
    int totalIv = 0;
    for (auto& p : m_ivTypes) totalIv += p.second;

    int totalPat = Database::instance().totalPatients();
    m_lblTotalPat->setText(QString::number(totalPat));
    m_lblTotalIv->setText(QString::number(totalIv));
    m_lblTotalRev->setText(QString::number(total, 'f', 2));
    m_lblAvgRev->setText(
        totalIv > 0 ? QString::number(total / totalIv, 'f', 2) : "0.00");

    update(); // trigger repaint
}

void StatisticsWidget::loadData()
{
    int year = m_yearCombo->currentData().toInt();
    m_monthlyRevenue.clear();
    m_monthlyCount.clear();
    m_ivTypes.clear();

    static const QStringList months = {
        "Jan","Feb","Mar","Apr","Maj","Jun",
        "Jul","Aug","Sep","Okt","Nov","Dec"
    };

    // Monthly revenue & count
    for (int m = 1; m <= 12; ++m) {
        QString from = QString("%1-%2-01").arg(year).arg(m, 2, 10, QChar('0'));
        QString to   = QDate(year, m, QDate(year,m,1).daysInMonth())
                           .toString("yyyy-MM-dd");
        QSqlQuery q(QSqlDatabase::database());
        q.prepare("SELECT COALESCE(SUM(cost),0), COUNT(*) FROM interventions "
                  "WHERE date(datetime) BETWEEN :f AND :t AND status='completed'");
        q.bindValue(":f", from);
        q.bindValue(":t", to);
        if (q.exec() && q.next()) {
            m_monthlyRevenue.append({months[m-1], q.value(0).toDouble()});
            m_monthlyCount.append({months[m-1],   q.value(1).toInt()});
        } else {
            m_monthlyRevenue.append({months[m-1], 0.0});
            m_monthlyCount.append({months[m-1],   0});
        }
    }

    // Intervention types
    QSqlQuery q2(QSqlDatabase::database());
    q2.prepare("SELECT intervention_type, COUNT(*) as cnt FROM interventions "
               "WHERE strftime('%Y', datetime)=:y "
               "GROUP BY intervention_type ORDER BY cnt DESC LIMIT 8");
    q2.bindValue(":y", QString::number(year));
    if (q2.exec()) {
        while (q2.next())
            m_ivTypes.append({q2.value(0).toString(), q2.value(1).toInt()});
    }
}

void StatisticsWidget::paintEvent(QPaintEvent*)
{
    if (m_monthlyRevenue.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int half = w / 2 - 12;

    // ── Bar chart: Monthly Revenue ────────────────────────────────────────────
    QRect barRect(24, 120, half, 340);
    drawBarChart(p, barRect, m_monthlyRevenue, "Prihodi po mjesecima (BAM)", "BAM");

    // ── Pie chart: Intervention types ─────────────────────────────────────────
    if (!m_ivTypes.isEmpty()) {
        QRect pieRect(w / 2 + 12, 120, half - 24, 340);
        drawPieChart(p, pieRect, m_ivTypes, "Vrste intervencija");
    }

    // ── Bar chart: Count per month ─────────────────────────────────────────
    QList<QPair<QString,double>> countAsDouble;
    for (auto& c : m_monthlyCount)
        countAsDouble.append({c.first, (double)c.second});

    if (h > 500) {
        QRect bar2(24, 480, w - 48, 280);
        drawBarChart(p, bar2, countAsDouble, "Broj intervencija po mjesecima", "");
    }
}

void StatisticsWidget::drawBarChart(QPainter& p,
                                     const QRect& r,
                                     const QList<QPair<QString,double>>& data,
                                     const QString& title,
                                     const QString& unit)
{
    if (data.isEmpty()) return;

    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(r, 8, 8);

    // Title
    p.setPen(QColor("#1565C0"));
    QFont tf("Segoe UI", 11, QFont::Bold);
    p.setFont(tf);
    p.drawText(QRect(r.x()+12, r.y()+10, r.width()-24, 24),
               Qt::AlignLeft | Qt::AlignVCenter, title);

    // Chart area
    int pad = 40;
    int chartX = r.x() + pad;
    int chartY = r.y() + 44;
    int chartW = r.width() - pad - 10;
    int chartH = r.height() - 60;

    double maxVal = 0;
    for (auto& d : data) maxVal = qMax(maxVal, d.second);
    if (maxVal <= 0) maxVal = 1;

    int n = data.size();
    double barW = (double)chartW / n * 0.65;
    double gap  = (double)chartW / n * 0.35;

    // Grid lines
    p.setPen(QPen(QColor("#EEEEEE"), 1));
    QFont gf("Segoe UI", 7);
    p.setFont(gf);
    for (int i = 0; i <= 4; ++i) {
        int y = chartY + chartH - (int)(chartH * i / 4.0);
        p.drawLine(chartX, y, chartX + chartW, y);
        double val = maxVal * i / 4.0;
        p.setPen(QColor("#9E9E9E"));
        p.drawText(QRect(r.x(), y-8, pad-4, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   val >= 1000 ? QString::number(val/1000,'f',1)+"k"
                               : QString::number((int)val));
        p.setPen(QPen(QColor("#EEEEEE"), 1));
    }

    // Bars
    for (int i = 0; i < n; ++i) {
        double val = data[i].second;
        int bh = (int)(chartH * val / maxVal);
        int bx = chartX + (int)(i * (barW + gap) + gap / 2);
        int by = chartY + chartH - bh;

        QColor col = CHART_COLORS[i % CHART_COLORS.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRoundedRect(bx, by, (int)barW, bh, 3, 3);

        // Value on top
        if (bh > 18) {
            p.setPen(Qt::white);
            QFont vf("Segoe UI", 7, QFont::Bold);
            p.setFont(vf);
            QString vs = val >= 1000 ? QString::number(val/1000,'f',1)+"k"
                                     : QString::number((int)val);
            p.drawText(QRect(bx, by+2, (int)barW, 14),
                       Qt::AlignCenter, vs);
        }

        // Label below
        p.setPen(QColor("#616161"));
        QFont lf("Segoe UI", 7);
        p.setFont(lf);
        p.drawText(QRect(bx-(int)(gap/2), chartY+chartH+2,
                         (int)(barW+gap), 14),
                   Qt::AlignCenter, data[i].first);
    }

    // Y-axis
    p.setPen(QPen(QColor("#BDBDBD"), 1));
    p.drawLine(chartX, chartY, chartX, chartY + chartH);
    p.drawLine(chartX, chartY + chartH, chartX + chartW, chartY + chartH);
}

void StatisticsWidget::drawPieChart(QPainter& p,
                                     const QRect& r,
                                     const QList<QPair<QString,int>>& data,
                                     const QString& title)
{
    if (data.isEmpty()) return;

    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(r, 8, 8);

    p.setPen(QColor("#1565C0"));
    QFont tf("Segoe UI", 11, QFont::Bold);
    p.setFont(tf);
    p.drawText(QRect(r.x()+12, r.y()+10, r.width()-24, 24),
               Qt::AlignLeft | Qt::AlignVCenter, title);

    int total = 0;
    for (auto& d : data) total += d.second;
    if (total == 0) return;

    // Pie area
    int pieSize = qMin(r.width() * 2 / 3, r.height() - 60);
    int px = r.x() + 10;
    int py = r.y() + 40;
    QRect pieR(px, py, pieSize, pieSize);

    double angle = 0;
    for (int i = 0; i < data.size(); ++i) {
        double span = 360.0 * data[i].second / total;
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(CHART_COLORS[i % CHART_COLORS.size()]);
        p.drawPie(pieR,
                  (int)(angle * 16),
                  (int)(span * 16));
        angle += span;
    }

    // Legend
    int lx = r.x() + pieSize + 20;
    int ly = r.y() + 44;
    QFont lf("Segoe UI", 8);
    p.setFont(lf);
    for (int i = 0; i < data.size(); ++i) {
        int pct = (int)(100.0 * data[i].second / total);
        p.setPen(Qt::NoPen);
        p.setBrush(CHART_COLORS[i % CHART_COLORS.size()]);
        p.drawRoundedRect(lx, ly + i*20, 10, 10, 2, 2);
        p.setPen(QColor("#424242"));
        QString txt = data[i].first;
        if (txt.length() > 16) txt = txt.left(14) + "..";
        p.drawText(lx+14, ly + i*20, r.width()-pieSize-30, 14,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   txt + "  " + QString::number(pct) + "%");
    }
}
