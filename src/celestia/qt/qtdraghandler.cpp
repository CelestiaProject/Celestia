#include "qtdraghandler.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QString>

#include <celestia/celestiacore.h>

namespace celestia::qt
{

namespace
{
auto
mouseEventPos(const QMouseEvent &m)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return m.globalPos();
#else
    return m.globalPosition();
#endif
}
} // end unnamed namespace

void
DragHandler::begin(const QMouseEvent &m, qreal s, int b)
{
    saveCursorPos = mouseEventPos(m);
    scale         = s;
    buttons       = b;
}

void
DragHandler::move(const QMouseEvent &m, qreal s)
{
    if (scale != s)
        begin(m, s, buttons);
    auto relativeMovement = mouseEventPos(m) - saveCursorPos;
    appCore->mouseMove(
        static_cast<float>(relativeMovement.x() * scale),
        static_cast<float>(relativeMovement.y() * scale),
        effectiveButtons());
    saveCursorPos = mouseEventPos(m);
}

void
DragHandler::setButton(int button)
{
    buttons |= button;
}

void
DragHandler::clearButton(int button)
{
    buttons &= ~button;
}

int
DragHandler::effectiveButtons() const
{
#ifdef __APPLE__
    // On the Mac, right dragging is be simulated with Option+left drag.
    // We may want to enable this on other platforms, though it's mostly only helpful
    // for users with single button mice.
    if (buttons & CelestiaCore::AltKey)
        return (buttons | CelestiaCore::RightButton) & (~CelestiaCore::LeftButton);
#endif
    return buttons;
}

// Warping drag handler

void
WarpingDragHandler::restoreCursorPosition() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCursor::setPos(saveCursorPos);
#else
    QCursor::setPos(saveCursorPos.toPoint());
#endif
}

void
WarpingDragHandler::move(const QMouseEvent &m, qreal s)
{
    if (scale != s)
        begin(m, s, buttons);
    auto relativeMovement = mouseEventPos(m) - saveCursorPos;
    appCore->mouseMove(
        static_cast<float>(relativeMovement.x() * scale),
        static_cast<float>(relativeMovement.y() * scale),
        effectiveButtons());

    restoreCursorPosition();
}

void
WarpingDragHandler::finish()
{
    restoreCursorPosition();
}

std::unique_ptr<DragHandler>
createDragHandler(CelestiaCore *appCore)
{
    if (QString platformName = QGuiApplication::platformName();
        platformName == "cocoa" ||
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
        platformName == "wayland" ||
#endif
        platformName == "windows" ||
        platformName == "xcb")
    {
        return std::make_unique<WarpingDragHandler>(appCore);
    }

    return std::make_unique<DragHandler>(appCore);
}

} // end namespace celestia::qt
