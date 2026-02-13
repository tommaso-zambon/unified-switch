# UnifiedSwitch — Developer Documentation

## Table of Contents

1. [Compilation](#compilation)
2. [Architecture Overview](#architecture-overview)
3. [Protocol Reference](#protocol-reference)
4. [Adding New Devices](#adding-new-devices)
5. [Debugging & Testing](#debugging--testing)
6. [Project Structure](#project-structure)

---

## Compilation

All compilation must be done in the **UCRT64** environment.

### Release Builds

```bash
# 1. Configure.exe - Device detection and setup wizard
gcc Configure.c -o Configure.exe -lhid -lsetupapi -O2 -s

# 2. LogiSwitch.exe - Core switching engine (console app)
gcc LogiSwitch.c -o LogiSwitch.exe -O2 -s

# 3. UnifiedSwitch.exe - Main hotkey daemon (GUI app, requires icon)
# First, compile resources (if icon/ folder exists)
rm -f icon/UnifiedSwitch.res
windres -O coff icon/UnifiedSwitch.rc icon/UnifiedSwitch.res

# Then build the executable
gcc UnifiedSwitch.c icon/UnifiedSwitch.res -o UnifiedSwitch.exe -mwindows -O2 -s

# 4. Debug utilities (console apps for testing)
gcc Configure_debug.c -o Configure_debug.exe -lhid -lsetupapi -O2 -s
gcc TestKeyboard.c -o TestKeyboard.exe -O2 -s
gcc TestMouse.c -o TestMouse.exe -O2 -s
```

### Compiler Flags Explained

| Flag         | Purpose                           |
| ------------ | --------------------------------- |
| `-O2`        | Optimize for speed                |
| `-s`         | Strip symbols (reduce size)       |
| `-mwindows`  | GUI subsystem (no console window) |
| `-lhid`      | Link HID library                  |
| `-lsetupapi` | Link Windows Setup API            |

### Debug Builds (with symbols)

Replace `-O2 -s` with `-g -O0` for debugging:

```bash
gcc Configure.c -o Configure_debug.exe -lhid -lsetupapi -g -O0
```

---

## Architecture Overview

### Component Interaction

```
┌─────────────────┐      ┌──────────────────┐      ┌─────────────────┐
│  UnifiedSwitch  │────▶│   LogiSwitch      │────▶│  HID Devices    │
│  (Hotkey Daemon)│      │  (Switch Engine) │      │ (KB/Mouse)      │
└─────────────────┘      └──────────────────┘      └─────────────────┘
        │                                                │
        │                       ┌──────────────────┐     │
        └──────────────────────▶│ ControlMyMonitor │────┘
                                │ (Monitor Switch) │
                                └──────────────────┘
```

### File Responsibilities

| File                | Type    | Purpose                                                         |
| ------------------- | ------- | --------------------------------------------------------------- |
| `UnifiedSwitch.c`   | GUI App | Low-level keyboard hook, hotkey handling, process orchestration |
| `LogiSwitch.c`      | Console | HID communication, protocol abstraction, device switching       |
| `Configure.c`       | GUI App | Device discovery, path ranking, initial setup wizard            |
| `Configure_debug.c` | Console | Verbose device enumeration for troubleshooting                  |
| `TestKeyboard.c`    | Console | Brute-force packet testing for unknown keyboards                |
| `TestMouse.c`       | Console | Brute-force packet testing for unknown mouses                   |

---

## Protocol Reference

### HID++ Protocol Basics

UnifiedSwitch uses **Logitech's HID++ protocol** over raw HID. This is the same protocol used by Logitech Options and Solaar.

**Key Concepts:**

- **Report ID 0x10**: Short packets (7 bytes)
- **Report ID 0x11**: Long packets (20 bytes) — used for channel switching
- **Device Index**: `0x00` for direct Bluetooth, `0xFF` for receiver
- **Sub ID 0x1E**: Channel switch command

### Bluetooth Channel Switch Packet

Standard 20-byte packet structure:

```
[0x11][0x00][ReportID][0x1E][Channel][padding...]
 │     │     │         │     │
 │     │     │         │     └── 0x00, 0x01, or 0x02 (channels 1-3)
 │     │     │         └──────── Command: Set Host (0x1E)
 │     │     └────────────────── Device-specific Report ID (0x09 or 0x0A)
 │     └──────────────────────── Device Index (0x00 for Bluetooth direct)
 └────────────────────────────── Long packet indicator (0x11)
```

### Report ID Mapping

Different devices use different Report IDs for the switch command:

| Device Family | Report ID | Example Devices        |
| ------------- | --------- | ---------------------- |
| Keyboards     | `0x09`    | MX Mechanical (0xB366) |
| Mice          | `0x0A`    | MX Master 3 (0xB033)   |

_These are discovered through testing with `TestKeyboard.exe` and `TestMouse.exe`_

### Device Path Format

Bluetooth HID paths contain the service UUID:

```
\\?\hid#{00001812-0000-1000-8000-00805f9b34fb}_dev_vid&02046d_pid&b366_...&col03#
```

The `col03` suffix indicates the HID collection that supports HID++ commands.

---

## Adding New Devices

### Step 1: Identify the Device

Run `Configure_debug.exe` to find your device's PID:

```bash
./Configure_debug.exe
# Look for: "PID: 0xB366" and "Category: KEYBOARD"
```

### Step 2: Find the Working Report ID

Use the test utilities to brute-force the correct packet format:

**For keyboards:**

1. Edit `TestKeyboard.c` and set `path` to your device's path from `config.ini`
2. Compile: `gcc TestKeyboard.c -o TestKeyboard.exe -O2 -s`
3. Ensure keyboard is on channel 1, run `./TestKeyboard.exe`
4. Check if keyboard switches to channel 2
5. Note which test number succeeded

**For mouses:**

1. Edit `TestMouse.c` with your mouse path
2. Compile and run `./TestMouse.exe`
3. Check which packet format switches the mouse

### Step 3: Update the Device Database

Add to `bt_devices[]` array in both `LogiSwitch.c` and `LogiSwitch_debug.c`:

```c
static const BT_Device bt_devices[] = {
    // Existing entries...
    {0xB366, 0x09, "MX Mechanical"},  // PID, Report ID, Name

    // Add your device here
    {0xXXXX, 0x09, "Your Device Name"},  // Replace 0xXXXX with actual PID
};
```

### Step 4: Test and Submit

1. Recompile `LogiSwitch.c`
2. Test switching with `UnifiedSwitch.exe`
3. Submit a PR with:
   - Device name
   - PID (from debug output)
   - Working Report ID
   - Confirmation of test results

---

## Debugging & Testing

### Troubleshooting Checklist

| Symptom                | Diagnostic                   | Solution                                               |
| ---------------------- | ---------------------------- | ------------------------------------------------------ |
| "No devices found"     | Run `Configure_debug.exe`    | Ensure devices are paired via Bluetooth, not receiver  |
| Switch fails silently  | Check `config.ini` paths     | Verify `col03` in keyboard path, `col02` in mouse path |
| Monitor doesn't switch | Test with PowerShell command | Verify DDC/CI support on monitor                       |
| Hotkeys not working    | Run as Administrator         | Low-level hooks require elevation                      |

### Debug Tools Usage

**Configure_debug.exe** — Device enumeration:

```bash
./Configure_debug.exe
# Shows all Logitech HID devices with:
# - PID and product name
# - Connection type (Bluetooth/Bolt/Unifying)
# - HID collection info (col01/col02/col03)
# - Full device paths
```

**TestKeyboard.exe / TestMouse.exe** — Protocol testing:

- Tests multiple packet formats sequentially
- Shows WriteFile success/failure
- Requires manual verification (did device switch?)

### Enabling Debug Output

Modify `LogiSwitch.c` to add verbose logging:

```c
// Add at top of switch_bluetooth()
printf("[DEBUG] Opening: %s\n", path);
printf("[DEBUG] Using Report ID: 0x%02X\n", report_id);
```

---

## Project Structure

```
UnifiedSwitch/
├── Configure.c              # Setup wizard (GUI)
├── Configure_debug.c        # Device scanner (console)
├── UnifiedSwitch.c          # Main hotkey daemon
├── LogiSwitch.c             # Core switching logic
├── LogiSwitch_debug.c       # Debug version (identical to LogiSwitch.c in provided files)
├── TestKeyboard.c           # Keyboard protocol tester
├── TestMouse.c              # Mouse protocol tester
├── config.ini               # User configuration (generated)
├── README.md                # User documentation
└── icon/
    ├── UnifiedSwitch.rc     # Windows resource script
    └── UnifiedSwitch.res    # Compiled resource (generated)
```

### Configuration File Format

```ini
[PATHS]
clickmon=dependencies\ControlMyMonitor.exe

[INTERFACES]
keyboard_path=\\?\hid#...&col03#...
mouse_path=\\?\hid#...&col02#...

[SOURCES]
device1=15    ; Monitor input for PC 1
device2=6     ; Monitor input for PC 2
device3=      ; Monitor input for PC 3 (optional)

[SETTINGS]
multiMonitor=0    ; 0=switch monitor, 1=don't switch
hotkeyMode=1      ; 1=Win+1/2/3, 2=Ctrl+Alt+1/2/3, 3=Ctrl+Shift+1/2/3
autoStart=1       ; 0=manual start, 1=registry startup
```

---

## References

- [Logitech HID++ 2.0 Specification](https://lekensteyn.nl/files/logitech/logitech_hidpp_2.0_specification_draft_2012-06-04.pdf)
- [Logitech HID++ 1.0 for Unifying Receivers](https://lekensteyn.nl/files/logitech/logitech_hidpp10_specification_for_Unifying_Receivers.pdf)
- [Logitech Public CPG Docs](https://github.com/Logitech/cpg-docs)
- [Solaar Project](https://github.com/pwr-Solaar/Solaar) — Linux HID++ implementation
- [hidpp Rust crate](https://docs.rs/hidpp) — Alternative protocol implementation

---

## License & Contribution

When contributing new device support:

1. Test thoroughly with `TestKeyboard.exe` or `TestMouse.exe`
2. Provide device name, PID, and confirmed working Report ID
3. Update this documentation if adding new protocol features

**Note:** This project uses Logitech's proprietary HID++ protocol. This is repo's based on public documentation and testing. Logitech's official documentation is available at [github.com/Logitech/cpg-docs](https://github.com/Logitech/cpg-docs).
