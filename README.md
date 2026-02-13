# UnifiedSwitch ![version](https://img.shields.io/badge/version-2.1.1-blue.svg)

Switch your Logitech MX keyboard, mouse, and monitor between multiple PCs instantly.
Just press Win + 1, Win + 2, or Win + 3 and everything jumps to the selected PC.

No more manual pairing to do!
Install once -> it just works forever.

## What's new in v2.1.1

- Wizard configuration
- Goodbye AutoHotkey! everything is pure C, speedy fast!
- RAM comsuption is now less of 1MB
- Supported new shortcuts

## Supported Connections

Logi Bolt is not supported right now, just direct bluetooth works.
As using Bolt adapter is actually faster than using bluetooth (and works on BIOS) i will try make switching possible

## Installation

1. [Download latest version](https://github.com/tommaso-zambon/unified-switch/releases/latest)
2. Execute `Configure.exe`<br>
   And follow wizard, it will create your `config.ini` file<br>
3. Put the required file in the right folder (optional, only needed if you want your monitor to switch automatically)<br>
   - Inside the UnifiedSwitch folder, open the `dependencies/` folder and place this file: [ControlMyMonitor.exe](https://www.nirsoft.net/utils/control_my_monitor.html)
   - [Follow this guide](#one-time-setup-find-your-monitor-input-numbers)
4. Run `UnifiedSwitch.exe` (just this time!)<br>
   That's it. You're done! ✔<br>
   Do the same on every PC you want to switch between.

## How to Use

Press:

- `⊞ Win + 1` -> switch everything to PC 1

- `⊞ Win + 2`-> switch everything to PC 2

- `⊞ Win + 3` -> switch everything to PC 3

> New shortcuts supported, you can select them from wizard

This will:

✔ switch your **keyboard**<br>
✔ switch your **mouse**<br>
✔ switch your **monitor input** (optional)<br>

### Monitor switching can be turned on/off

> you can follow wizard guide in Configure.exe instead

Inside `config.ini`:

- `multiMonitor = 0`-> switch monitor input **automatically**

- `multiMonitor = 1` -> **do not** switch monitor input (good for multi-monitor setups)

If the monitor never changes input, your screen does not support DDC/CI.

## One-Time Setup: Find Your Monitor Input Numbers

Do this only **once per Monitor**.

1. Open **PowerShell** (no administrator rights needed)
2. Copy & paste this command:

```powershell
1..20 | ForEach-Object { Write-Host "Testing input $_ (watch monitor – Ctrl+C to stop)" -ForegroundColor Yellow; & "C:\Macros\UnifiedSwitch\dependencies\ControlMyMonitor.exe" /SetValue "\\.\DISPLAY1\Monitor0" 60 $_; Start-Sleep -Seconds 2 }
```

Your screen will change input a few times.

3. When it switches, write down the number you saw<br>
   Examples:
   - input 5 => HDMI-1
   - input 6 => HDMI-2
   - input 15 => DisplayPort

4. Add these numbers to your config.ini

```ini
[SOURCES]
device1=5    ; HDMI-1
device2=6    ; HDMI-2
device3=   ; (OPTIONAL)
```

`device1` is linked to logitech channel 1, so at win+1 source switches there, same for `device2` and `device3`, but with channel 2 and 3

Save it.
Now `Win+1` / `Win+2` / `Win+3` will switch PC and Logitech channels instantly.

> [!NOTE]
> While the switch logic is written in portable C, the implementation uses Windows HID APIs. It could work on other OSes with modification, but I won't be doing that work.

### for devs && contributions

If you would like to contribute, here are [docs](TECHNICAL_README.md)

## huge thanks

- nirsoft/ControlMyMonitor
