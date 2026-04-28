#pragma once
#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QVector>

class SignatureWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SignatureWidget(QWidget* parent = nullptr);

    bool    isEmpty() const;
    QPixmap getSignature() const;
    bool    saveToFile(const QString& path) const;
    void    loadFromFile(const QString& path);

public slots:
    void clear();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;

private:
    QPixmap m_canvas;
    QPoint  m_lastPos;
    bool    m_drawing = false;
    bool    m_empty   = true;
};
