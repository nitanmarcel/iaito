#include "CustomTitleBar.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_MACOS
void configureMacTitleBar(QWidget *window);
#endif

WindowControlButton::WindowControlButton(Kind kind, QWidget *parent)
    : QAbstractButton(parent)
    , m_kind(kind)
{
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::ArrowCursor);
    setAttribute(Qt::WA_Hover, true);
}

void WindowControlButton::setMaximized(bool maximized)
{
    if (m_maximized != maximized) {
        m_maximized = maximized;
        update();
    }
}

QSize WindowControlButton::sizeHint() const
{
    const int h = qMax(24, fontMetrics().height() + 12);
    return QSize(h + 14, h);
}

void WindowControlButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const bool hover = underMouse();
    const bool pressed = isDown();
    const QPalette pal = palette();
    QColor fg = pal.color(QPalette::WindowText);
    if (hover || pressed) {
        QColor bg = m_kind == Close ? QColor(0xe8, 0x1c, 0x2e) : pal.color(QPalette::Highlight);
        if (pressed) {
            bg = bg.darker(115);
        }
        p.fillRect(rect(), bg);
        fg = m_kind == Close ? QColor(Qt::white) : pal.color(QPalette::HighlightedText);
    }

    p.setPen(QPen(fg, 1.2));
    const QRectF r = rect();
    const qreal cx = r.center().x();
    const qreal cy = r.center().y();
    const qreal s = 4.5;
    switch (m_kind) {
    case Minimize:
        p.drawLine(QPointF(cx - s, cy), QPointF(cx + s, cy));
        break;
    case MaximizeRestore:
        if (m_maximized) {
            p.drawRect(QRectF(cx - s + 1.5, cy - s + 1.5, 2 * s - 3, 2 * s - 3));
            QPainterPath path;
            path.moveTo(cx - s + 1.5, cy - s);
            path.lineTo(cx + s, cy - s);
            path.lineTo(cx + s, cy + s - 1.5);
            p.drawPath(path);
        } else {
            p.drawRect(QRectF(cx - s, cy - s, 2 * s, 2 * s));
        }
        break;
    case Close:
        p.drawLine(QPointF(cx - s, cy - s), QPointF(cx + s, cy + s));
        p.drawLine(QPointF(cx - s, cy + s), QPointF(cx + s, cy - s));
        break;
    }
}

CustomTitleBar::CustomTitleBar(QWidget *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setObjectName(QStringLiteral("customTitleBar"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_icon = new QLabel(this);
    m_icon->setPixmap(window->windowIcon().pixmap(16, 16));
    m_title = new QLabel(window->windowTitle(), this);
    m_title->setTextInteractionFlags(Qt::NoTextInteraction);

    auto *left = new QHBoxLayout;
    left->setSpacing(6);
    left->setContentsMargins(0, 0, 0, 0);
    left->addWidget(m_icon);
    left->addStretch(1);

    auto *right = new QHBoxLayout;
    right->setSpacing(0);
    right->setContentsMargins(0, 0, 0, 0);
    right->addStretch(1);

#ifdef Q_OS_MACOS
    m_max = nullptr;
    m_icon->hide();
#else
    auto *minBtn = new WindowControlButton(WindowControlButton::Minimize, this);
    m_max = new WindowControlButton(WindowControlButton::MaximizeRestore, this);
    auto *closeBtn = new WindowControlButton(WindowControlButton::Close, this);
    connect(minBtn, &QAbstractButton::clicked, this, [this]() { m_window->showMinimized(); });
    connect(m_max, &QAbstractButton::clicked, this, &CustomTitleBar::toggleMaximize);
    connect(closeBtn, &QAbstractButton::clicked, this, [this]() { m_window->close(); });
    right->addWidget(minBtn);
    right->addWidget(m_max);
    right->addWidget(closeBtn);
#endif

    auto *layout = new QHBoxLayout(this);
#ifdef Q_OS_MACOS
    layout->setContentsMargins(72, 0, 8, 0);
#else
    layout->setContentsMargins(8, 0, 0, 0);
#endif
    layout->setSpacing(0);
    layout->addLayout(left, 1);
    layout->addWidget(m_title, 0, Qt::AlignCenter);
    layout->addLayout(right, 1);

    m_window->installEventFilter(this);
    refreshState();
}

void CustomTitleBar::toggleMaximize()
{
    if (m_window->isMaximized()) {
        m_window->showNormal();
    } else {
        m_window->showMaximized();
    }
}

void CustomTitleBar::refreshState()
{
    m_title->setText(m_window->windowTitle());
    m_icon->setPixmap(m_window->windowIcon().pixmap(16, 16));
    if (m_max) {
        m_max->setMaximized(m_window->isMaximized());
    }
}

void CustomTitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_window->windowHandle()) {
        m_window->windowHandle()->startSystemMove();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CustomTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        toggleMaximize();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CustomTitleBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::Window));
    p.setPen(palette().color(QPalette::Mid));
    p.drawLine(rect().bottomLeft(), rect().bottomRight());
}

bool CustomTitleBar::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    switch (event->type()) {
    case QEvent::WindowTitleChange:
    case QEvent::WindowIconChange:
    case QEvent::WindowStateChange:
        refreshState();
        break;
    default:
        break;
    }
    return false;
}

FramelessResizeFilter::FramelessResizeFilter(QWidget *window, int margin)
    : QObject(window)
    , m_window(window)
    , m_margin(margin)
{}

Qt::Edges FramelessResizeFilter::edgesAt(const QPoint &p) const
{
    Qt::Edges e;
    if (p.x() <= m_margin) {
        e |= Qt::LeftEdge;
    }
    if (p.x() >= m_window->width() - m_margin) {
        e |= Qt::RightEdge;
    }
    if (p.y() <= m_margin) {
        e |= Qt::TopEdge;
    }
    if (p.y() >= m_window->height() - m_margin) {
        e |= Qt::BottomEdge;
    }
    return e;
}

static Qt::CursorShape cursorForEdges(Qt::Edges e)
{
    const bool l = e & Qt::LeftEdge, r = e & Qt::RightEdge;
    const bool t = e & Qt::TopEdge, b = e & Qt::BottomEdge;
    if ((l && t) || (r && b)) {
        return Qt::SizeFDiagCursor;
    }
    if ((r && t) || (l && b)) {
        return Qt::SizeBDiagCursor;
    }
    if (l || r) {
        return Qt::SizeHorCursor;
    }
    if (t || b) {
        return Qt::SizeVerCursor;
    }
    return Qt::ArrowCursor;
}

bool FramelessResizeFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_window || m_window->isMaximized()) {
        return false;
    }
    switch (event->type()) {
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (!(me->buttons() & Qt::LeftButton)) {
            m_window->setCursor(cursorForEdges(edgesAt(me->pos())));
        }
        break;
    }
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            const Qt::Edges e = edgesAt(me->pos());
            if (e && m_window->windowHandle()) {
                m_window->windowHandle()->startSystemResize(e);
                return true;
            }
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void applyCustomFrame(QMainWindow *window)
{
    auto *top = new QWidget(window);
    auto *topLayout = new QVBoxLayout(top);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    topLayout->addWidget(new CustomTitleBar(window, top));
#ifndef Q_OS_MACOS
    topLayout->addWidget(window->menuBar());
#endif
    window->setMenuWidget(top);

#ifdef Q_OS_MACOS
    window->winId();
    configureMacTitleBar(window);
#else
    window->setWindowFlag(Qt::FramelessWindowHint);
    const int gutter = 6;
    window->setContentsMargins(gutter, 0, gutter, gutter);
    window->setMouseTracking(true);
    window->installEventFilter(new FramelessResizeFilter(window, gutter));
#endif
}
