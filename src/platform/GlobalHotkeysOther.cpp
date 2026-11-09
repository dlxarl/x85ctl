#include "platform/GlobalHotkeys.h"
namespace kcc {
GlobalHotkeys& GlobalHotkeys::instance() { static GlobalHotkeys g; return g; }
bool GlobalHotkeys::supported() { return false; }
bool GlobalHotkeys::has(int) const { return false; }
bool GlobalHotkeys::add(int, quint32, quint32) { return false; }
void GlobalHotkeys::remove(int) {}
quint32 GlobalHotkeys::modsFromQt(int) { return 0; }
}
