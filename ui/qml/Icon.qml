import QtQuick
import QtQuick.Shapes

// Stroked line icons on a 24×24 grid, tinted by `color` (no emoji / font glyphs).
Item {
    id: ic
    property string name
    property color color: Theme.dim
    property real size: 14
    implicitWidth: size; implicitHeight: size

    readonly property var paths: ({
        brush:   "M20 4 L11 13 M9.5 10.5 L13.5 14.5 M9.5 10.5 C6.5 10.5 5 12.5 5 15 C5 17 4 18.5 2.5 19.5 C5.5 21 9.5 20.5 11.5 18.5 C13 17 13.5 15.5 13.5 14.5",
        eraser:  "M4 21 L20 21 M8 18 L4.5 14.5 C3.9 13.9 3.9 13.1 4.5 12.5 L12.5 4.5 C13.1 3.9 13.9 3.9 14.5 4.5 L19.5 9.5 C20.1 10.1 20.1 10.9 19.5 11.5 L13 18 Z M9 8 L16 15",
        pipette: "M3 21 L4 17 L12.5 8.5 L15.5 11.5 L7 20 Z M11 7 L17 13 M13.5 5.5 L15.5 3.5 C16.5 2.5 18 2.5 19 3.5 L20.5 5 C21.5 6 21.5 7.5 20.5 8.5 L18.5 10.5",
        grid:    "M3.5 5 L8 5 L8 9.5 L3.5 9.5 Z M10 5 L14.5 5 L14.5 9.5 L10 9.5 Z M16.5 5 L21 5 L21 9.5 L16.5 9.5 Z M3.5 12.5 L8 12.5 L8 17 L3.5 17 Z M10 12.5 L14.5 12.5 L14.5 17 L10 17 Z M16.5 12.5 L21 12.5 L21 17 L16.5 17 Z",
        clock:   "M12 3 A9 9 0 1 1 11.99 3 Z M12 7 L12 12 L15.5 14",
        mouse:   "M12 3 C8.5 3 6 5.5 6 9 L6 15 C6 18.5 8.5 21 12 21 C15.5 21 18 18.5 18 15 L18 9 C18 5.5 15.5 3 12 3 Z M12 3 L12 9.5 M6 9.5 L18 9.5",
        down:    "M12 4 L12 19 M6 13 L12 19 L18 13",
        up:      "M12 20 L12 5 M6 11 L12 5 L18 11",
        home:    "M3.5 11 L12 3.5 L20.5 11 M5.5 9.5 L5.5 20 L18.5 20 L18.5 9.5 M10 20 L10 14 L14 14 L14 20",
        sun:     "M12 8 A4 4 0 1 1 11.99 8 Z M12 2.5 L12 4.5 M12 19.5 L12 21.5 M2.5 12 L4.5 12 M19.5 12 L21.5 12 M5.3 5.3 L6.7 6.7 M17.3 17.3 L18.7 18.7 M5.3 18.7 L6.7 17.3 M17.3 6.7 L18.7 5.3",
        keys:    "M3 7 L21 7 L21 18 L3 18 Z M6.5 10.5 L7.5 10.5 M10.5 10.5 L11.5 10.5 M14.5 10.5 L15.5 10.5 M17.5 10.5 L18 10.5 M6.5 14.5 L17.5 14.5",
        macro:   "M8 5 L19 12 L8 19 Z",
        layers:  "M12 3.5 L21 8.5 L12 13.5 L3 8.5 Z M3 12.5 L12 17.5 L21 12.5 M3 16.5 L12 21.5 L21 16.5",
        info:    "M12 3 A9 9 0 1 1 11.99 3 Z M12 11 L12 17 M12 7.5 L12 8",
        snake:   "M4 18 L10 18 A3 3 0 0 0 10 12 L14 12 A3 3 0 0 1 14 6 L19 6 M19 6 L19.01 6",
        spark:   "M12 3 L13.8 10.2 L21 12 L13.8 13.8 L12 21 L10.2 13.8 L3 12 L10.2 10.2 Z"
    })

    Shape {
        anchors.centerIn: parent
        width: 24; height: 24; scale: ic.size / 24
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: ic.color; strokeWidth: 2; fillColor: "transparent"
            capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
            PathSvg { path: ic.paths[ic.name] || "" }
        }
    }
}
