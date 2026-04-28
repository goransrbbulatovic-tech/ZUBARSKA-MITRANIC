#pragma once
#include <QWidget>
#include <QList>
#include <QPair>

class QComboBox;
class QLabel;

class StatisticsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StatisticsWidget(QWidget* parent = nullptr);
    void refresh();

private:
    void setupUi();
    void loadData();

    // Chart helpers
    void drawBarChart(QPainter& p, const QRect& r,
                      const QList<QPair<QString,double>>& data,
                      const QString& title, const QString& unit);
    void drawPieChart(QPainter& p, const QRect& r,
                      const QList<QPair<QString,int>>& data,
                      const QString& title);

    // Data
    QList<QPair<QString,double>> m_monthlyRevenue;   // month -> BAM
    QList<QPair<QString,int>>    m_ivTypes;          // type -> count
    QList<QPair<QString,int>>    m_monthlyCount;     // month -> count

    // Summary labels
    QLabel* m_lblTotalRev    = nullptr;
    QLabel* m_lblTotalIv     = nullptr;
    QLabel* m_lblTotalPat    = nullptr;
    QLabel* m_lblAvgRev      = nullptr;

    QComboBox* m_yearCombo   = nullptr;

protected:
    void paintEvent(QPaintEvent* e) override;
};
