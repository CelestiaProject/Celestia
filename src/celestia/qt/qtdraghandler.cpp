#include "qtdraghandler.h"

#include <QCursor>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QString>

#include <celestia/celestiacore.h>

namespace celestia::qt
{

void
DragHandler::begin(const QMouseEvent &m, qreal s, int b)
{
    saveCursorPos = m.globalPosition();
    scale         = s;
    buttons       = b;
}

void
DragHandler::move(const QMouseEvent &m, qreal s)
{
    if (scale != s)
        begin(m, s, buttons);
    auto relativeMovement = m.globalPosition() - saveCursorPos;
    appCore->mouseMove(
        static_cast<float>(relativeMovement.x() * scale),
        static_cast<float>(relativeMovement.y() * scale),
        effectiveButtons());
    saveCursorPos = m.globalPosition();
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
    QCursor::setPos(saveCursorPos.toPoint());
}

void
WarpingDragHandler::move(const QMouseEvent &m, qreal s)
{
    if (scale != s)
        begin(m, s, buttons);
    auto relativeMovement = m.globalPosition() - saveCursorPos;
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
