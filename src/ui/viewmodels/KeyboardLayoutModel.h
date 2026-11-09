#pragma once
#include <QObject>
#include <QVariantList>

namespace kcc {
// Exposes the X85 key geometry to QML (shared by Lighting Studio and Key Mapping).
class KeyboardLayoutModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList keys READ keys CONSTANT)
    Q_PROPERTY(int canvasWidth READ canvasWidth CONSTANT)
    Q_PROPERTY(int canvasHeight READ canvasHeight CONSTANT)
public:
    using QObject::QObject;
    QVariantList keys() const;
    int canvasWidth() const;
    int canvasHeight() const;
};
}
