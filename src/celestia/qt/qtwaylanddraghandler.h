#pragma once

#include "qtdraghandler.h"

#include <QWidget>
#include <memory>
#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#include <wayland-client.h>

class WaylandDragHandler : public DragHandler
{
public:
    struct PointerInterfaces
    {
        wl_registry                     *registry;
        zwp_pointer_constraints_v1      *pointerConstraints{ nullptr };
        zwp_relative_pointer_manager_v1 *relativePointerManager{ nullptr };

        PointerInterfaces(wl_registry *);
        ~PointerInterfaces();
    };

    WaylandDragHandler(QWidget *, CelestiaCore *);
    ~WaylandDragHandler();

    void begin(const QMouseEvent &, qreal, int) override;
    void move(const QMouseEvent &, qreal) override;
    void finish() override;

private:
    QWidget                           *widget{ nullptr };
    std::shared_ptr<PointerInterfaces> pointerInterfaces{ nullptr };
    int                                buttons{ 0 };
    wl_surface                        *surface{ nullptr };
    wl_pointer                        *pointer{ nullptr };
    zwp_relative_pointer_v1           *relativePointer{ nullptr };
    zwp_locked_pointer_v1             *lockedPointer{ nullptr };
    bool                               fallback{ false };

    static const zwp_relative_pointer_v1_listener listener;

    static void processRelativePointer(
        void *data,
        zwp_relative_pointer_v1 *,
        std::uint32_t,
        std::uint32_t,
        wl_fixed_t,
        wl_fixed_t,
        wl_fixed_t,
        wl_fixed_t);
};
