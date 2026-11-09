// x85ctl: command-line verification tool for the ATTACK SHARK X85 protocol.
// Every write command is a DRY RUN that only prints packets, unless --send is given.
#include "core/TransactionRunner.h"
#include "hid/DeviceDiscovery.h"
#include "hid/HidDevice.h"
#include "protocol/CommandBuilder.h"
#include "protocol/ProtocolParser.h"
#include "protocol/X85Layout.h"
#include <QCoreApplication>
#include <QStringList>
#include <QThread>
#include <cstdio>

using namespace kcc;

static void usage() {
    std::puts(
        "x85ctl <command> [options]\n"
        "  list                       enumerate supported devices and HID collections\n"
        "  descriptor                 dump the vendor collection report descriptor\n"
        "  get                        one GetFeature on the vendor collection (no write)\n"
        "  read-colors   [--send]     04 F5 read-back of per-key colours (sends 04 F5, 04 02)\n"
        "  light <mode>  [--brightness 0-5] [--speed 0-5] [--dir 0-3] [--color RRGGBB] [--colorful] [--send]\n"
        "  fill <RRGGBB> [--brightness 0-5] [--send]   per-key: every key one colour (mode 0x80)\n"
        "  modes                      list lighting modes\n"
        "Without --send nothing is written; the exact packet sequence is printed instead.");
}

static void printTx(const Transaction& tx) {
    for (const auto& p : tx)
        std::printf("  SETF %s%s\n", p.data.toHex(' ').constData(), p.readReply ? "   (+GETF)" : "");
}

static HidDevice* openConfig() {
    const auto devs = DeviceDiscovery::scan();
    if (devs.isEmpty()) { std::fprintf(stderr, "No supported device found.\n"); return nullptr; }
    const auto* c = devs.first().configInterface();
    if (!c) { std::fprintf(stderr, "Vendor collection 0xFF13/0x0001 not found.\n"); return nullptr; }
    auto* d = new HidDevice;
    if (!d->open(c->path)) { std::fprintf(stderr, "Open failed: %s\n", qPrintable(d->lastError())); delete d; return nullptr; }
    return d;
}

static TransactionRunner runnerFor(HidDevice& d) {
    return TransactionRunner(d, [](int ms) { QThread::msleep(ms); },
                             [](const QString& s) { std::printf("  %s\n", qPrintable(s)); });
}

static int runTx(const Transaction& tx, bool send) {
    if (!send) { std::puts("Dry run (add --send to write):"); printTx(tx); return 0; }
    std::unique_ptr<HidDevice> d(openConfig());
    if (!d) return 1;
    const auto r = runnerFor(*d).run(tx);
    std::printf(r.ok ? "OK\n" : "FAILED: %s\n", qPrintable(r.error));
    return r.ok ? 0 : 1;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QStringList a = app.arguments().mid(1);
    if (a.isEmpty()) { usage(); return 1; }
    const QString cmd = a.takeFirst();
    const bool send = a.removeAll("--send") > 0;
    const bool colorful = a.removeAll("--colorful") > 0;
    auto opt = [&](const QString& k, int def) {
        const int i = a.indexOf(k);
        return (i >= 0 && i + 1 < a.size()) ? a[i + 1].toInt(nullptr, 0) : def;
    };
    auto optS = [&](const QString& k) { const int i = a.indexOf(k); return (i >= 0 && i + 1 < a.size()) ? a[i + 1] : QString(); };

    if (cmd == "list") {
        const auto devs = DeviceDiscovery::scan();
        if (devs.isEmpty()) { std::puts("No supported device."); return 1; }
        for (const auto& d : devs) {
            std::printf("%s  %04X:%04X  rev %04X  \"%s\" / \"%s\"\n", qPrintable(d.model), d.vendorId, d.productId,
                        d.releaseBcd, qPrintable(d.manufacturer), qPrintable(d.product));
            for (const auto& i : d.interfaces)
                std::printf("  iface %d  usage %04X/%04X%s\n", i.interfaceNumber, i.usagePage, i.usage,
                            i.role == InterfaceRole::Config ? "  <- config" : "");
        }
        return 0;
    }
    if (cmd == "modes") {
        for (const auto& m : x85::kModes) std::printf("%3d  %-11s caps 0x%02X\n", m.id, m.name, m.caps);
        return 0;
    }
    if (cmd == "descriptor" || cmd == "get") {
        std::unique_ptr<HidDevice> d(openConfig());
        if (!d) return 1;
        if (cmd == "descriptor") {
            const auto raw = d->reportDescriptor();
            std::printf("%s\n\n%s", raw.toHex(' ').constData(), qPrintable(ProtocolParser::parseDescriptor(raw).text));
        } else {
            const auto r = d->getFeature();
            std::printf("GETF (%lld): %s\n", qlonglong(r.size()), r.toHex(' ').constData());
        }
        return 0;
    }
    if (cmd == "read-colors") {
        if (!send) { std::puts("Dry run (add --send):"); printTx(CommandBuilder::readPerKeyColorsRequest());
                     std::puts("  GETF ×(1+9)\n  SETF 04 02 …"); return 0; }
        std::unique_ptr<HidDevice> d(openConfig());
        if (!d) return 1;
        QString err;
        const auto t = runnerFor(*d).readPerKeyColors(&err);
        if (t.isEmpty()) { std::printf("FAILED: %s\n", qPrintable(err)); return 1; }
        const auto m = ProtocolParser::parseColorTable(t);
        for (const auto& k : x85::layout())
            if (m.contains(k.lightIndex))
                std::printf("%-6s #%02X%02X%02X\n", k.label, m[k.lightIndex].r, m[k.lightIndex].g, m[k.lightIndex].b);
        return 0;
    }
    if (cmd == "light" && !a.isEmpty()) {
        LightingState s;
        s.mode = uint8_t(a.first().toInt(nullptr, 0));
        if (!x85::findMode(s.mode)) { std::fprintf(stderr, "Unknown mode (see `x85ctl modes`)\n"); return 1; }
        s.brightness = uint8_t(qBound(0, opt("--brightness", x85::kDefaultBrightness), x85::kBrightnessMax));
        s.speed = uint8_t(qBound(0, opt("--speed", x85::kDefaultSpeed), x85::kSpeedMax));
        s.direction = x85::Direction(qBound(0, opt("--dir", 0), 3));
        s.colorful = colorful;
        if (const QString c = optS("--color"); !c.isEmpty()) {
            const uint v = c.toUInt(nullptr, 16);
            s.r = uint8_t(v >> 16); s.g = uint8_t(v >> 8); s.b = uint8_t(v);
        }
        return runTx(CommandBuilder::lighting(s), send);
    }
    if (cmd == "fill" && !a.isEmpty()) {
        const uint v = a.first().toUInt(nullptr, 16);
        QMap<int, Rgb> m;
        for (const auto& k : x85::layout()) m[k.lightIndex] = Rgb{uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)};
        return runTx(CommandBuilder::perKeyColors(m, uint8_t(opt("--brightness", x85::kDefaultBrightness))), send);
    }
    usage();
    return 1;
}
