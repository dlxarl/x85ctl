#include "KeyboardLayoutModel.h"
#include "protocol/X85Layout.h"
#include <QVariantMap>

namespace kcc {
// Key rectangles in the pixel space of ui/assets/x85.png, so QML can overlay them on the photo.
QVariantList KeyboardLayoutModel::keys() const {
    QVariantList l;
    for (const auto& k : x85::layout())
        l << QVariantMap{{"label", QString::fromUtf8(k.label)}, {"keyIndex", k.keyIndex}, {"lightIndex", k.lightIndex},
                         {"x", k.x}, {"y", k.y}, {"w", k.w}, {"h", k.h},
                         {"remappable", k.hidUsage != 0xAF}};
    return l;
}
int KeyboardLayoutModel::canvasWidth() const { return x85::kCanvasWidth; }
int KeyboardLayoutModel::canvasHeight() const { return x85::kCanvasHeight; }
}
