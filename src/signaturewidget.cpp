#include "signaturewidget.h"
#include <QPainter>
#include <QMouseEvent>

SignatureWidget::SignatureWidget(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(120);
    setMinimumWidth(300);
    setCursor(Qt::CrossCursor);
    setStyleSheet("background:#fff; border:2px dashed #1565C0; border-radius:6px;");
}

void SignatureWidget::resizeEvent(QResizeEvent*)
{
    if (m_canvas.isNull() || m_canvas.size() != size()) {
        QPixmap tmp(size());
        tmp.fill(Qt::white);
        if (!m_canvas.isNull())
            QPainter(&tmp).drawPixmap(0,0,m_canvas);
        m_canvas = tmp;
    }
}

void SignatureWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_drawing = true;
        m_lastPos = e->pos();
        m_empty   = false;
    }
}

void SignatureWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_drawing) return;
    if (m_canvas.isNull()) return;
    QPainter p(&m_canvas);
    QPen pen(QColor("#1565C0"), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawLine(m_lastPos, e->pos());
    m_lastPos = e->pos();
    update();
}

void SignatureWidget::mouseReleaseEvent(QMouseEvent*)
{
    m_drawing = false;
}

void SignatureWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (!m_canvas.isNull())
        p.drawPixmap(0, 0, m_canvas);
    if (m_empty) {
        p.setPen(QColor("#9E9E9E"));
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(rect(), Qt::AlignCenter, "Potpišite se ovdje...");
    }
}

void SignatureWidget::clear()
{
    if (!m_canvas.isNull())
        m_canvas.fill(Qt::white);
    m_empty = true;
    update();
}

bool SignatureWidget::isEmpty() const { return m_empty; }

QPixmap SignatureWidget::getSignature() const { return m_canvas; }

bool SignatureWidget::saveToFile(const QString& path) const
{
    if (m_empty || m_canvas.isNull()) return false;
    return m_canvas.save(path, "PNG");
}

void SignatureWidget::loadFromFile(const QString& path)
{
    if (m_canvas.load(path)) {
        m_empty = false;
        update();
    }
}
