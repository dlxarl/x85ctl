#include "hid/Platform.h"
#include <hidapi.h>
#include <hidapi_darwin.h>
namespace kcc::platform {
// Non-exclusive open: macOS keeps driving the keyboard; we only talk to the vendor collection.
void prepareHid() { hid_darwin_set_open_exclusive(0); }
const char* name() { return "macOS"; }
}
