#include "platform/GlobalHotkeys.h"
#import <Carbon/Carbon.h>
#include <QHash>
#include <Qt>

namespace kcc {
static QHash<int, EventHotKeyRef>& refs() { static QHash<int, EventHotKeyRef> r; return r; }

static OSStatus onHotKey(EventHandlerCallRef, EventRef ev, void*) {
    EventHotKeyID hk;
    if (GetEventParameter(ev, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof hk, nullptr, &hk) != noErr) return noErr;
    const int id = int(hk.id);
    QMetaObject::invokeMethod(&GlobalHotkeys::instance(), [id] { emit GlobalHotkeys::instance().activated(id); }, Qt::QueuedConnection);
    return noErr;
}

GlobalHotkeys& GlobalHotkeys::instance() { static GlobalHotkeys g; return g; }
bool GlobalHotkeys::supported() { return true; }
bool GlobalHotkeys::has(int id) const { return refs().contains(id); }

bool GlobalHotkeys::add(int id, quint32 vk, quint32 mods) {
    static bool installed = false;
    if (!installed) {
        const EventTypeSpec spec{kEventClassKeyboard, kEventHotKeyPressed};
        if (InstallApplicationEventHandler(&onHotKey, 1, &spec, nullptr, nullptr) != noErr) return false;
        installed = true;
    }
    remove(id);
    EventHotKeyRef ref = nullptr;
    const EventHotKeyID hk{'KCCH', UInt32(id)};
    if (RegisterEventHotKey(vk, mods, hk, GetApplicationEventTarget(), 0, &ref) != noErr || !ref) return false;
    refs().insert(id, ref);
    return true;
}

void GlobalHotkeys::remove(int id) {
    if (auto ref = refs().take(id)) UnregisterEventHotKey(ref);
}

quint32 GlobalHotkeys::modsFromQt(int m) {
    quint32 c = 0;
    if (m & Qt::ControlModifier) c |= cmdKey;      // Qt on macOS: Control = ⌘
    if (m & Qt::MetaModifier) c |= controlKey;     // Meta = ⌃
    if (m & Qt::AltModifier) c |= optionKey;
    if (m & Qt::ShiftModifier) c |= shiftKey;
    return c;
}
}
