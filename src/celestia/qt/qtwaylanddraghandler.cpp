#include "qtwaylanddraghandler.h"

#include <QGuiApplication>
#include <cstdint>
#include <cstring>
#include <memory>
#include <qpa/qplatformnativeinterface.h>

namespace
{

QPlatformNativeInterface *
getPlatformNativeInterface()
{
    static QPlatformNativeInterface *pni = nullptr;
    if (!pni)
    {
        pni = QGuiApplication::platformNativeInterface();
    }

    return pni;
}

void
addRegistryItem(
    void         *data,
    wl_registry  *registry,
    std::uint32_t name,
    const char   *interface,
    std::uint32_t version)
{
    auto pointerInterfaces = static_cast<WaylandDragHandler::PointerInterfaces *>(data);
    if (std::strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0
        && version == zwp_pointer_constraints_v1_interface.version)
    {
        pointerInterfaces->pointerConstraints = static_cast<zwp_pointer_constraints_v1 *>(
            wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, version));
    }
    else if (
        std::strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0
        && version == zwp_relative_pointer_manager_v1_interface.version)
    {
        pointerInterfaces->relativePointerManager = static_cast<zwp_relative_pointer_manager_v1 *>(
            wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, version));
    }
}

void
removeRegistryItem(void *data, wl_registry *registry, std::uint32_t name)
{
    // not handled
}

std::shared_ptr<WaylandDragHandler::PointerInterfaces>
getPointerInterfaces()
{
    static std::weak_ptr<WaylandDragHandler::PointerInterfaces> cachedPointerInterfaces{};
    auto pointerInterfaces = cachedPointerInterfaces.lock();
    if (pointerInterfaces)
        return pointerInterfaces;

    QPlatformNativeInterface *pni = getPlatformNativeInterface();
    if (!pni)
        return nullptr;

    auto display = static_cast<wl_display *>(pni->nativeResourceForIntegration("wl_display"));
    if (!display)
        return nullptr;

    wl_registry *registry = wl_display_get_registry(display);
    if (!registry)
        return nullptr;

    pointerInterfaces = std::make_shared<WaylandDragHandler::PointerInterfaces>(registry);
    static const wl_registry_listener registryListener{ addRegistryItem, removeRegistryItem };

    wl_registry_add_listener(registry, &registryListener, pointerInterfaces.get());
    wl_display_roundtrip(display);

    cachedPointerInterfaces = pointerInterfaces;
    return pointerInterfaces;
}

} // end unnamed namespace

const zwp_relative_pointer_v1_listener WaylandDragHandler::listener{ processRelativePointer };

WaylandDragHandler::WaylandDragHandler(QWidget *_widget, CelestiaCore *_appCore) :
    DragHandler(_appCore), widget(_widget)
{
}

WaylandDragHandler::~WaylandDragHandler()
{
    if (lockedPointer)
        zwp_locked_pointer_v1_destroy(lockedPointer);
    if (relativePointer)
        zwp_relative_pointer_v1_destroy(relativePointer);
}

void
WaylandDragHandler::begin(const QMouseEvent &m, qreal s, int b)
{
    buttons  = b;
    scale    = s;
    auto pni = getPlatformNativeInterface();
    if (!pni)
    {
        fallback = true;
        DragHandler::begin(m, s, b);
        return;
    }

    surface = static_cast<wl_surface *>(
        pni->nativeResourceForWindow("surface", widget->window()->windowHandle()));
    pointer = static_cast<wl_pointer *>(pni->nativeResourceForIntegration("wl_pointer"));
    if (!surface || !pointer)
    {
        fallback = true;
        DragHandler::begin(m, s, b);
        return;
    }

    pointerInterfaces = getPointerInterfaces();

    relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(
        pointerInterfaces->relativePointerManager,
        pointer);
    if (!relativePointer)
    {
        fallback = true;
        DragHandler::begin(m, s, b);
        return;
    }

    zwp_relative_pointer_v1_add_listener(relativePointer, &listener, this);

    lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
        pointerInterfaces->pointerConstraints,
        surface,
        pointer,
        nullptr,
        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT);
    if (!lockedPointer)
    {
        zwp_relative_pointer_v1_destroy(relativePointer);
        fallback = true;
        DragHandler::begin(m, s, b);
        return;
    }
}

void
WaylandDragHandler::move(const QMouseEvent &m, qreal s)
{
    if (fallback)
    {
        DragHandler::move(m, s);
    }
}

void
WaylandDragHandler::finish()
{
    if (fallback)
    {
        DragHandler::finish();
        return;
    }
    zwp_locked_pointer_v1_destroy(lockedPointer);
    lockedPointer = nullptr;

    zwp_relative_pointer_v1_destroy(relativePointer);
    relativePointer = nullptr;
}

void
WaylandDragHandler::processRelativePointer(
    void *data,
    zwp_relative_pointer_v1 * /* pointer */,
    std::uint32_t /* utime_hi */,
    std::uint32_t /* utime_lo */,
    wl_fixed_t dx,
    wl_fixed_t dy,
    wl_fixed_t /* dx_unaccel */,
    wl_fixed_t /* dy_unaccel */)
{
    auto dragHandler = static_cast<WaylandDragHandler *>(data);
    dragHandler->appCore->mouseMove(
        static_cast<float>(wl_fixed_to_double(dx) * dragHandler->scale),
        static_cast<float>(wl_fixed_to_double(dy) * dragHandler->scale),
        dragHandler->effectiveButtons());
}

WaylandDragHandler::PointerInterfaces::PointerInterfaces(wl_registry *_registry) :
    registry{ _registry }
{
}

WaylandDragHandler::PointerInterfaces::~PointerInterfaces()
{
    if (pointerConstraints)
        zwp_pointer_constraints_v1_destroy(pointerConstraints);
    if (relativePointerManager)
        zwp_relative_pointer_manager_v1_destroy(relativePointerManager);
    if (registry)
        wl_registry_destroy(registry);
}
