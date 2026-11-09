#pragma once
class QWindow;
namespace kcc {
// Hides the native title bar and lets the app's own top bar act as one. On macOS the
// traffic-light buttons stay (native behaviour, full screen, accessibility) and are centred
// vertically in a bar of `barHeight` px starting `leftInset` px from the left edge.
// Other platforms: no-op (the regular frame is kept).
void applyWindowChrome(QWindow* window, int barHeight, int leftInset);
}
