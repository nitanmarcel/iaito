#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include <QAbstractButton>
#include <QWidget>

class QLabel;
class QMainWindow;

void applyCustomFrame(QMainWindow *window);

class WindowControlButton : public QAbstractButton
{
    Q_OBJECT

public:
    enum Kind { Minimize, MaximizeRestore, Close };

    explicit WindowControlButton(Kind kind, QWidget *parent = nullptr);

    void setMaximized(bool maximized);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Kind m_kind;
    bool m_maximized = false;
};

class CustomTitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit CustomTitleBar(QWidget *window, QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void toggleMaximize();
    void refreshState();

    QWidget *m_window;
    QLabel *m_icon;
    QLabel *m_title;
    WindowControlButton *m_max;
};

class FramelessResizeFilter : public QObject
{
    Q_OBJECT

public:
    explicit FramelessResizeFilter(QWidget *window, int margin = 6);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Qt::Edges edgesAt(const QPoint &pos) const;

    QWidget *m_window;
    int m_margin;
};

#endif // CUSTOMTITLEBAR_H
