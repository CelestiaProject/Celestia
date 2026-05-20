#include "qtwaylanddraghandler.h"

#include <cstring>

#include <QGuiApplication>
#include <QWidget>
#include <QWindow>
#include <qpa/qplatformnativeinterface.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0) && defined(HAS_WAYLAND_POINTER_WARP)
#include <pointer-warp-v1-client-protocol.h>
#endif

#include <celestia/celestiacore.h>

namespace celestia::qt::wayland
{

namespace
{

struct Interfaces
{
    wl_pointer* pointer{ nullptr };
#ifdef HAS_WAYLAND_POINTER_CONSTRAINTS
    zwp_pointer_constraints_v1* pointerConstraints{ nullptr };
    zwp_relative_pointer_manager_v1* relativePointerManager{ nullptr };
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0) && defined(HAS_WAYLAND_POINTER_WARP)
    bool hasPointerWarp{ false };
#endif
    bool initialized{ false };
};

void
addRegistryItem(
    void         *data,
    wl_registry  *registry,
    std::uint32_t name,
    const char   *interface,
    std::uint32_t version)
{
    [[maybe_unused]] auto interfaces = static_cast<Interfaces*>(data);
#ifdef HAS_WAYLAND_POINTER_CONSTRAINTS
    if (std::strcmp(interface, zwp_pointer_constraints_v1_interface.name) == 0)
    {
        if (version == static_cast<std::uint32_t>(zwp_pointer_constraints_v1_interface.version))
        {
            interfaces->pointerConstraints = static_cast<zwp_pointer_constraints_v1*>(
                wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, version));
        }
        return;
    }

    if (std::strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
    {
        if (version == static_cast<std::uint32_t>(zwp_relative_pointer_manager_v1_interface.version))
        {
            interfaces->relativePointerManager = static_cast<zwp_relative_pointer_manager_v1*>(
                wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, version));
        }
        return;
    }
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0) && defined(HAS_WAYLAND_POINTER_WARP)
    if (std::strcmp(interface, wp_pointer_warp_v1_interface.name) == 0)
    {
        if (version == static_cast<std::uint32_t>(wp_pointer_warp_v1_interface.version))
            interfaces->hasPointerWarp = true;
        return;
    }
#endif
}

void
removeRegistryItem(void *data, wl_registry *registry, std::uint32_t name)
{
    // not handled
}

wl_registry_listener
createListener()
{
    wl_registry_listener listener;
    listener.global = &addRegistryItem;
    listener.global_remove = &removeRegistryItem;
    return listener;
}

const Interfaces*
getInterfaces()
{
    static Interfaces interfaces;
    if (interfaces.initialized)
        return &interfaces;

    interfaces.initialized = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto instance = static_cast<const QGuiApplication*>(QGuiApplication::instance());
    if (!instance)
        return &interfaces;

    const auto* waylandApp = instance->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!waylandApp)
        return &interfaces;

    auto display = waylandApp->display();
    interfaces.pointer = waylandApp->pointer();
#else
    QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
    if (!pni)
        return &interfaces;

    auto display = static_cast<wl_display*>(pni->nativeResourceForIntegration("wl_display"));
    interfaces.pointer = static_cast<wl_pointer*>(pni->nativeResourceForIntegration("wl_pointer"));
#endif

    if (!display)
        return &interfaces;

    wl_registry* registry = wl_display_get_registry(display);
    if (!registry)
        return &interfaces;

    static const wl_registry_listener listener = createListener();
    wl_registry_add_listener(registry, &listener, &interfaces);
    wl_display_roundtrip(display);

    return &interfaces;
}

} // end unnamed namespace

#ifdef HAS_WAYLAND_POINTER_CONSTRAINTS
const zwp_relative_pointer_v1_listener PointerConstraintsDragHandler::s_listener{ &processRelativePointer };

PointerConstraintsDragHandler::PointerConstraintsDragHandler(CelestiaCore* appCore,
                                                             QWidget* widget,
                                                             wl_pointer* pointer) :
    DragHandler(appCore), m_widget(widget), m_pointer(pointer)
{
}

void
PointerConstraintsDragHandler::begin(const QMouseEvent &m, qreal s, int b)
{
    DragHandler::begin(m, s, b);

    auto pni = QGuiApplication::platformNativeInterface();
    if (!pni)
    {
        m_fallback = true;
        return;
    }

    auto surface = static_cast<wl_surface*>(pni->nativeResourceForWindow("surface", m_widget->window()->windowHandle()));
    if (!surface)
    {
        m_fallback = true;
        return;
    }

    auto interfaces = getInterfaces();
    m_relativePointer = UniqueRelativePointer(zwp_relative_pointer_manager_v1_get_relative_pointer(interfaces->relativePointerManager,
                                                                                                   m_pointer));

    if (!m_relativePointer)
    {
        m_fallback = true;
        return;
    }

    zwp_relative_pointer_v1_add_listener(m_relativePointer.get(), &s_listener, this);

    m_lockedPointer = UniqueLockedPointer(zwp_pointer_constraints_v1_lock_pointer(interfaces->pointerConstraints,
                                                                                  surface,
                                                                                  m_pointer,
                                                                                  nullptr,
                                                                                  ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_ONESHOT));
    if (!m_lockedPointer)
    {
        m_relativePointer.reset();
        m_fallback = true;
    }
}

void
PointerConstraintsDragHandler::move(const QMouseEvent &m, qreal s)
{
    if (m_fallback)
        DragHandler::move(m, s);
}

void
PointerConstraintsDragHandler::finish()
{
    if (m_fallback)
    {
        DragHandler::finish();
        return;
    }

    m_lockedPointer.reset();
    m_relativePointer.reset();
}

void
PointerConstraintsDragHandler::processRelativePointer(
    void *data,
    zwp_relative_pointer_v1 * /* pointer */,
    std::uint32_t /* utime_hi */,
    std::uint32_t /* utime_lo */,
    wl_fixed_t dx,
    wl_fixed_t dy,
    wl_fixed_t /* dx_unaccel */,
    wl_fixed_t /* dy_unaccel */)
{
    auto dragHandler = static_cast<PointerConstraintsDragHandler*>(data);
    dragHandler->appCore()->mouseMove(static_cast<float>(wl_fixed_to_double(dx) * dragHandler->scale()),
                                      static_cast<float>(wl_fixed_to_double(dy) * dragHandler->scale()),
                                      dragHandler->effectiveButtons());
}
#endif

std::unique_ptr<DragHandler> createWaylandDragHandler([[maybe_unused]] QWidget* widget, CelestiaCore* appCore)
{
    [[maybe_unused]] auto interfaces = getInterfaces();
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0) && defined(HAS_WAYLAND_POINTER_WARP)
    // use built-in Qt pointer warp functionality
    if (interfaces->hasPointerWarp)
        return std::make_unique<WarpingDragHandler>(appCore);
#endif
#ifdef HAS_WAYLAND_POINTER_CONSTRAINTS
    if (interfaces->pointer && interfaces->pointerConstraints && interfaces->relativePointerManager)
        return std::make_unique<PointerConstraintsDragHandler>(appCore, widget, interfaces->pointer);
#endif

    return std::make_unique<DragHandler>(appCore);
}

} // end namespace celestia::qt::wayland
