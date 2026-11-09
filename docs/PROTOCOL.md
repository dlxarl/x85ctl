# ATTACK SHARK X85 (SONIX 0C45:8006): configuration protocol

How this document was produced:
- **Hardware:** passive inspection of a physical unit with `ioreg`, `hidutil` and `hid_get_report_descriptor`.
- **Vendor app:** `ATTACK SHARK X85 SOFT.exe` (Inno Setup, SHA-256 `a8c868f1ab845a4dc08699761f4c337071d93c4beac80addd897bee0d39d0e11`). It was unpacked with `innoextract`, and `app/DeviceDriver.exe` was decompiled with **Ghidra 12.2** (headless, all 5460 functions). Nothing was executed.

Confidence tags:
- **[HW]** observed on hardware.
- **[RE]** read directly from decompiled code.
- **[INF]** inferred from naming or UI and not yet confirmed.

The Sharkfin documentation (VID 0x3151, vendor page 0xFFFF, opcodes `0x8F`/`0x07`/`0x87`…) describes a **different protocol family**. Its opcodes do not appear in this vendor app and must not be sent to this device.

---

## 1. Transport

| Item | Value |
|---|---|
| VID / PID [HW][RE] | `0x0C45` / `0x8006` (vendor `config.xml`: `VID_0C45&PID_8006`) |
| 2.4 GHz dongle [RE] | `0x05AC` / `0x024F` (listed in `config.xml`, not tested, not supported here) |
| Config interface [HW] | Usage page `0xFF13`, usage `0x0001` (USB interface 2) |
| Report descriptor [HW] | `06 13 ff 09 01 a1 01 15 00 26 ff 00 75 08 95 40 09 02 81 02 09 03 91 02 0a 00 ff b1 02 c0` |
| Reports [HW] | 64-byte IN, OUT and FEATURE, no report ID |
| Used by vendor app [RE] | **Feature reports only**: `HidD_SetFeature(h, buf, 0x41)` and `IOCTL_HID_GET_FEATURE (0xB0192)`. hidapi is statically linked (`FUN_004516c0` = `lookup_functions`). |

Every packet is 64 bytes, prefixed with report ID `0x00` (65 bytes on the wire through hidapi). The vendor app does not use the IN/OUT reports for configuration.

### Pacing [RE]
- `cmd_delaytime` = **35 ms** in `layouts/rgb-keyboard.xml`. It is stored at `transport+0x30` (default 10) and slept before each packet (`FUN_0044faa0`).
- **Single packet:** `Sleep(delay)` → SetFeature → (optional) `Sleep(delay)` → GetFeature (the reply/ack).
- **Multi-packet payload** (> 0x41 bytes): split into 64-byte pages. Each page is a SetFeature followed by `Sleep(delay)`. On failure it retries up to 10 times.

### Reply [RE]
The GetFeature reply echoes the packet. Byte 3 is a status: `0x01` = data follows (read commands), `0xFF` = busy, so the vendor app resends (`FUN_0044fc90`).

## 2. Framing [RE]

Packet = `[0x04, cmd, b2, 0, 0, 0, 0, 0, pages, 0 …]` (64 bytes).

A write transaction always looks like this:

```
04 18                     begin          (0x19 for macros)
04 <cmd> .. [8]=N         command header, N = number of 64-byte data pages
<N × 64-byte data pages>  last 2 bytes of the payload block = AA 55
04 02                     end
04 F0                     apply/save     (after lighting & keymap; not after settings/clock)
```

The `AA 55` trailer goes at offset `N*64 - 2` of the data block (bytes `0xAA, 0x55`).

## 3. Commands

| Cmd | Name | Header | Data | Source |
|---|---|---|---|---|
| `0x18` | Begin | `04 18` | — | all writers [RE] |
| `0x19` | Begin (macro) | `04 19` | — | `FUN_0042db70` [RE] |
| `0x02` | End | `04 02` | — | [RE] |
| `0xF0` | Apply | `04 F0` | — | [RE] |
| `0x13` | Lighting mode | `04 13 00 00 00 00 00 00 01` | 1 page, §4 | `FUN_0042b400`, `FUN_0044c4e0`, `FUN_004346b0` [RE] |
| `0x23` | Per-key colours | `04 23 … [8]=09` | 9 pages (576 B), §5 | `FUN_0044c5f0`, `FUN_00435060` [RE] |
| `0x11` | Keymap, base layer | `04 11 … [8]=09` | 9 pages, §6 | `FUN_00418590(0)` [RE] |
| `0x27` | Keymap, Fn layer | `04 27 … [8]=09` | 9 pages, §6 | `FUN_00418590(1)` [RE] |
| `0x17` | Keyboard settings | `04 17 01 … [8]=01` | 1 page, §7 | `FUN_00413ff0` [RE] |
| `0x15` | Macros | `04 19`, `04 15 … [8]=N`, data, `04 02` (no `04 F0`) | N pages, §8 | `FUN_0042db70` [RE] |
| `0x28` | Clock | `04 28 … [8]=01` | 1 page | `FUN_00423ac0` [RE] (screen models) |
| `0x72` | Screen image | `04 72 [2]=slot … [8..9]=N` | 4 KiB chunks | `FUN_00422b00` [RE] (screen models; X85 has `screen="0"`) |
| `0xF5` | **Read** per-key colours | `04 F5 … [8]=09` | GetFeature × 9 | `FUN_0042b1e0` [RE] |
| `0x20` | **Read** custom-light table (single-colour boards only) | `04 20 … [8]=03/08` | GetFeature × N | `FUN_00428110`, `FUN_00432e80` [RE] |

**What can be read back:** the vendor app reads state only with `0xF5` (per-key RGB table, read when the custom-light page opens: `FUN_0042b1e0` → `FUN_0044ff70`) and `0x20` (single-colour boards). There is **no read command for the effect (mode/brightness/speed/direction/colour), the keymap or the settings**. The vendor app keeps those in its own database. This app does the same: it reads `0xF5` on connect and autosaves every edit into the active profile.

No firmware-version read command exists in this app. Firmware update is not implemented in it either (`<firmware version="" file="">`).

## 4. Lighting mode (`0x13`) [RE]

Data page:

| Byte | Field (`t_light_data` column) |
|---|---|
| 0 | mode |
| 1 | R (COLORREF low byte) |
| 2 | G |
| 3 | B |
| 8 | colorful (1 = rainbow) |
| 9 | brightness 0…5 (`brightness_max`) |
| 10 | speed 0…5 (`speed_max`) |
| 11 | direction: 0 = left, 1 = right, 2 = up, 3 = down |
| 14,15 | `AA 55` |

### Modes (`FUN_0042b6b0`, default table)
`config_func` bits (`FUN_0042ae40`):
- 0x01 brightness
- 0x02 speed
- 0x04 direction left/right
- 0x08 direction up/down
- 0x10 RGB colour
- 0x20 colour button + "colorful" checkbox

| Mode | Name (lang id) | config_func |
|---|---|---|
| 0 | LED Off (540) | 0x00 |
| 1 | Static (521) | 0x31 |
| 2 | SingleOn (522) | 0x33 |
| 3 | SingleOff (523) | 0x33 |
| 4 | Glittering (524) | 0x33 |
| 5 | Falling (525) | 0x33 |
| 6 | Colourful (526) | 0x03 |
| 7 | Breath (527) | 0x33 |
| 8 | Spectrum (528) | 0x03 |
| 9 | Outward (529) | 0x33 |
| 10 | Scrolling (530) | 0x3B |
| 11 | Rolling (531) | 0x37 |
| 12 | Rotating (532) | 0x37 |
| 13 | Explode (533) | 0x33 |
| 14 | Launch (534) | 0x33 |
| 15 | Ripples (535) | 0x33 |
| 16 | Flowing (536) | 0x37 |
| 17 | Pulsating (537) | 0x33 |
| 18 | Tilt (538) | 0x37 |
| 19 | Shuttle (539) | 0x33 |
| 0x80 | User (per-key) | brightness only: data `[0]=0x80, [9]=brightness` (`FUN_0044c4e0`) |

Defaults from `rgb-keyboard.xml`: mode 11, brightness 5, speed 3.

## 5. Per-key colours (`0x23`, RGB board, `rgb_keyboard="1"`) [RE]

The vendor app first sets mode `0x80` (§4), then runs a full transaction with `04 23` and 9 pages. The 576-byte block holds 144 slots of 4 bytes: `[light_index, R, G, B]`. Bytes 574–575 are `AA 55`. Unused slots are zero. `light_index` comes from the layout (§9).

After the table the vendor app sends `04 02` and `04 F0`, **both followed by a GetFeature** (unlike the lighting transaction, whose `04 F0` is not read back).

**This app's live mode (not vendor behaviour) [INF]:** once the board is in user mode `0x80` with the same brightness, live updates send only the `04 23` transaction and skip the mode switch, because re-sending `0x80` makes the whole board blink. Explicit *Apply* always sends the full vendor sequence.

**Colour accuracy:** the vendor app sends raw RGB bytes (no gamma or white balance in `DeviceDriver.exe`). This app optionally applies `out = 255·gain·(in/255)^γ` with defaults γ = 1.0 and gains 1.00 / 0.69 / 0.94 (γ = 2.2 was the first default, but it crushed dim channels so blue→violet gradients looked flat blue on the board) (FastLED `TypicalSMD5050` = 255/176/240), which the user can calibrate against white.

Read-back (`0xF5`): send `04 F5 … [8]=09`, then GetFeature. Data pages follow if byte 3 of the reply is `0x01`. Then send `04 02`. The pages are decoded as `[idx, R, G, B]`.

## 6. Keymap (`0x11` base / `0x27` Fn) [RE]

144 slots × 4 bytes at `key_index*4`. `AA 55` at 574. An all-zero slot means **default** (the vendor app only fills remapped keys).

| Slot | Meaning |
|---|---|
| `00 00 00 00` | default |
| `02 mods usage 00` | keyboard key. `mods` = HID modifier bitmask (01 LCtrl, 02 LShift, 04 LAlt, 08 LGUI…). A modifier key alone is encoded as `02 <bit> 00 00`. Shortcuts seen: Win+D `02 08 07`, Win+E `02 08 08`, Win+L `02 08 0F`, Ctrl+C `02 01 06`, Ctrl+V `02 01 19`, Ctrl+X `02 01 1B`, Ctrl+W `02 01 1A`, Alt+Tab `02 04 2B` |
| `03 lo hi 00` | consumer usage (LE). Play/Pause `CD`, Stop `B7`, Prev `B6`, Next `B5`, Vol+ `E9`, Vol− `EA`, Mute `E2` |
| `01 01 btn 00` | mouse button: 01 left, 02 right, 04 middle, 08 back, 10 forward (03 seen = double?) |
| `01 03 delta 00` | mouse wheel: `01` up, `FF` down |
| `06 idx ? mode` | macro (type 3) |
| `07 a b c` | type 13 (light function), [INF] |
| `05 02 x 00` / `05 03 00 00` | special functions (types 8–11 / type 1), [INF] |

## 7. Settings (`0x17`) [RE + INF]

Data page:
- byte 0 = 0, byte 1 = 1.
- bytes 2/3/4 = three checkboxes. [INF] They are most likely "Disable Win key" (160), "Disable Alt+F4" (161) and "Disable Alt+Tab" (162). The exact order is not proven.
- byte 5 = `fn_switch`.
- byte 6 = `sleep_time`. [INF] Probably 0 = never, 1 = 1 min, 2 = 5 min, 3 = 30 min.
- byte 8 = `key_respondtime`, debounce level 1…5.
- bytes 62–63 = `AA 55`.

## 8. Macros (`0x15`) [RE: `FUN_0042db70`, key slot `FUN_00418590`] — implemented, not yet tried on hardware

Sequence: `04 19` (macro begin, **not** `04 18`) → `04 15 … [8]=pages` → `pages × 64` data bytes (data pages are not read back) → `04 02`. **No `04 F0`** afterwards. Delay between packets as usual (35 ms).

Data block (buffer `0xE00` = 3584 bytes, zero-filled):

| Offset | Content |
|---|---|
| `0 … 399` | index table, 100 entries × 4 bytes: `u16 offset` (LE) + `00 00`. A macro with no events is `FF FF FF FF`. Entries past the last macro stay `00 00 00 00`. |
| first body at `400` | each body: `u16 event_count`, 6 zero bytes, then `event_count × 4` bytes. The next body starts at `offset + 8 + 4·count`. |
| end | `AA 55` in the last two bytes of the last page |

`pages = len/64 + 1` if `len` is a multiple of 64, otherwise `len/64 + 2` (the vendor code always adds one spare page), written to byte 8 of the `04 15` header.

Event = `[delay_lo, delay_hi, usage, type]`:

| type | meaning | other bytes |
|---|---|---|
| `B0` | key down | `usage` = HID usage |
| `30` | key up | `usage` |
| `90` | mouse button down | `usage` = `01` left, `02` right, `04` middle |
| `10` | mouse button up | same |
| `50` | delay | `u16` milliseconds, **minimum 10** |

The vendor database records a macro as typed steps (`t_macrorecord`: 1 delay, 2 key down, 3 key up, 4/5 mouse down/up); key and mouse events carry no delay of their own, delays are separate steps.

**Binding a key to a macro** (keymap slot, §6): `06 index mode count`. `index` is the macro's position in the table above. `mode` and `count` are the vendor's `macro_value2` / `macro_value3`, taken from the key dialog (combo: *Play once*, *Play multiple times* (loop count 1–255 edit box), *Stop when key is pressed again*, *Release to stop*). Only "multiple times" = `1` with a count is proven by the decompile; `0` once, `2` toggle, `3` hold are **[INF]** (combo order) and `count` is `0` for the other modes. The macro table goes to the keyboard before the keymap that refers to it.

Limits used here: 100 macros, buffer 3584 bytes → at most 56 pages. This app refuses a table that does not fit.

## 9. Layout

From `layouts/rgb-keyboard.xml`: 81 keys, 6 rows, `key_index == light_index` for every key (max 121). The table lives in `src/protocol/X85Layout.cpp`.

## 10. Verification status

| Item | Status |
|---|---|
| Discovery, descriptor, idle GetFeature (64 zero bytes) | **verified on hardware** [HW] |
| Per-key colour upload (`04 18`, `04 23`, `04 02`, `04 F0`, mode `0x80` switch) | **works on hardware** [HW]: used daily by the author (editor, *Live* painting, Snake) |
| Per-key colour read-back (`04 F5`) | **works on hardware** [HW]: 142 slots come back on connect |
| Lighting effects (`04 13`) | built byte for byte from the vendor code [RE]; no failure reported |
| Keymap base / Fn (`04 11` / `04 27`) | implemented, byte-exact tests; **not confirmed on hardware** |
| Macros (`04 19`, `04 15`) and key slot `06 idx mode count` | implemented, byte-exact tests; **not confirmed on hardware**. Play mode `1` (repeat N) is [RE]; `0`, `2`, `3` are [INF] |
| Keyboard settings (`04 17`) | not exposed in the UI |

Observed on hardware: one full per-key transaction takes about 0.5 s; re-sending the mode switch `0x80` makes the board blink, so live updates skip it once the board is in user mode [INF: this skip is the app's own behaviour, not the vendor's].
