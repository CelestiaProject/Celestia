#pragma once

#include <cstdint>
#include <memory>

#include <QtGlobal>

#include <wayland-client.h>
#ifdef HAS_WAYLAND_POINTER_CONSTRAINTS
#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <relative-pointer-unstable-v1-client-protocol.h>
#endif

#include <celutil/uniquedel.h>
#include "qtdraghandler.h"

class QMouseEvent;
class QWidget;

class CelestiaCore;

namespace celestia::qt::wayland
{
#ifdef HAS_WAYLAND_POINTER_CONSTRAINTS
using UniqueRelativePointer = util::UniquePtrDel<zwp_relative_pointer_v1, &zwp_relative_pointer_v1_destroy>;
using UniqueLockedPointer = util::UniquePtrDel<zwp_locked_pointer_v1, &zwp_locked_pointer_v1_destroy>;

class PointerConstraintsDragHandler final : public DragHandler
{
public:
    PointerConstraintsDragHandler(CelestiaCore*, QWidget*, wl_pointer*);

    void begin(const QMouseEvent&, qreal, int) override;
    void move(const QMouseEvent&, qreal) override;
    void finish() override;

private:
    static const zwp_relative_pointer_v1_listener s_listener;

    QWidget* m_widget;
    wl_pointer* m_pointer;
    UniqueRelativePointer m_relativePointer;
    UniqueLockedPointer m_lockedPointer;
    bool m_fallback{ false };

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
#endif

std::unique_ptr<DragHandler> createWaylandDragHandler(QWidget*, CelestiaCore*);

} // end namespace celestia::qt::wayland
