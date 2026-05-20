#include "qtdraghandler.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QString>

#include <celestia/celestiacore.h>

#ifdef HAS_WAYLAND_DRAG
#include "qtwaylanddraghandler.h"
#endif

namespace celestia::qt
{

namespace
{
inline auto
mouseEventPos(const QMouseEvent& m)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return m.globalPos();
#else
    return m.globalPosition();
#endif
}
}

void
DragHandler::begin(const QMouseEvent& m, qreal s, int b)
{
    m_saveCursorPos = mouseEventPos(m);
    m_scale         = s;
    m_buttons       = b;
}

void
DragHandler::move(const QMouseEvent &m, qreal s)
{
    if (m_scale != s)
        begin(m, s, m_buttons);
    auto relativeMovement = mouseEventPos(m) - m_saveCursorPos;
    m_appCore->mouseMove(static_cast<float>(relativeMovement.x() * m_scale),
                         static_cast<float>(relativeMovement.y() * m_scale),
                         effectiveButtons());
    m_saveCursorPos = mouseEventPos(m);
}

void
DragHandler::setButton(int button)
{
    m_buttons |= button;
}

void
DragHandler::clearButton(int button)
{
    m_buttons &= ~button;
}

int
DragHandler::effectiveButtons() const
{
#ifdef __APPLE__
    // On the Mac, right dragging is be simulated with Option+left drag.
    // We may want to enable this on other platforms, though it's mostly only helpful
    // for users with single button mice.
    if (m_buttons & CelestiaCore::AltKey)
        return (m_buttons | CelestiaCore::RightButton) & (~CelestiaCore::LeftButton);
#endif
    return m_buttons;
}

// Warping drag handler

void
WarpingDragHandler::restoreCursorPosition() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCursor::setPos(saveCursorPos());
#else
    QCursor::setPos(saveCursorPos().toPoint());
#endif
}

void
WarpingDragHandler::move(const QMouseEvent &m, qreal s)
{
    DragHandler::move(m, s);
    restoreCursorPosition();
}

void
WarpingDragHandler::finish()
{
    restoreCursorPosition();
}

std::unique_ptr<DragHandler>
createDragHandler([[maybe_unused]] QWidget *widget, CelestiaCore *appCore)
{
    QString platformName = QGuiApplication::platformName();

    if (platformName == "cocoa" || platformName == "windows" || platformName == "xcb")
        return std::make_unique<WarpingDragHandler>(appCore);

#ifdef HAS_WAYLAND_DRAG
    if (platformName == "wayland")
        return wayland::createWaylandDragHandler(widget, appCore);
#elif QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    // Qt 6.10+ has built-in support for the Wayland pointer warp protocol
    // so try to use that
    if (platformName == "wayland")
        return std::make_unique<WarpingDragHandler>(appCore);
#endif

    return std::make_unique<DragHandler>(appCore);
}

} // end namespace celestia::qt
