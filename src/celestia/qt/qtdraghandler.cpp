#include "qtdraghandler.h"

#include <QCursor>
#include <QGuiApplication>
#ifdef USE_WAYLAND
#include "qtwaylanddraghandler.h"
#endif

void
DragHandler::begin(const QMouseEvent &m, qreal s, int b)
{
    saveCursorPos = m.globalPos();
    scale         = s;
    buttons       = b;
}

void
DragHandler::move(const QMouseEvent &m, qreal s)
{
    if (scale != s)
        begin(m, s, buttons);
    auto relativeMovement = m.globalPos() - saveCursorPos;
    appCore->mouseMove(
        static_cast<float>(relativeMovement.x() * scale),
        static_cast<float>(relativeMovement.y() * scale),
        effectiveButtons());
    saveCursorPos = m.globalPos();
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
WarpingDragHandler::move(const QMouseEvent &m, qreal s)
{
    if (scale != s)
        begin(m, s, buttons);
    auto relativeMovement = m.globalPos() - saveCursorPos;
    appCore->mouseMove(
        static_cast<float>(relativeMovement.x() * scale),
        static_cast<float>(relativeMovement.y() * scale),
        effectiveButtons());
    QCursor::setPos(saveCursorPos);
}

void
WarpingDragHandler::finish()
{
    QCursor::setPos(saveCursorPos);
}

std::unique_ptr<DragHandler>
createDragHandler([[maybe_unused]] QWidget *widget, CelestiaCore *appCore)
{
    QString platformName = QGuiApplication::platformName();

    if (platformName == "cocoa" || platformName == "windows" || platformName == "xcb")
        return std::make_unique<WarpingDragHandler>(appCore);

#ifdef USE_WAYLAND
    if (platformName == "wayland")
        return std::make_unique<WaylandDragHandler>(widget, appCore);
#endif

    return std::make_unique<DragHandler>(appCore);
}
