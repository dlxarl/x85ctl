#pragma once
namespace kcc::platform {
void prepareHid();          // per-OS HIDAPI options, called once after hid_init
const char* name();
}
