# UnifiedSwitch

Switch your Logitech MX keyboard, mouse, and monitor between multiple PCs instantly.
Just press Win + 1, Win + 2, or Win + 3 and everything jumps to the selected PC.

No more manual pairing to do!
Install once -> it just works forever.

## Installation

1. [Download latest version](https://github.com/tommaso-zambon/unified-switch/releases/latest)
2. Put the required files in the right folder<br>
   Inside the UnifiedSwitch folder, open the `dependencies/` folder and place these two files inside:
   - [hidapitester.exe](https://github.com/todbot/hidapitester/releases/latest)
   - [ControlMyMonitor.exe](https://www.nirsoft.net/utils/control_my_monitor.html) (optional, only needed if you want your monitor to switch automatically)
3. Install it<br>
   Right-click `install.bat` -> run as administrator.

   - This makes UnifiedSwitch start automatically each time your PC starts.

4. Restart your PC<br>
   (or run UnifiedSwitch.exe manually once)

That's it. You're done! ✔<br>
Do the same on every PC you want to switch between.

To remove it completely, run `uninstall.bat`.

## How to Use

Press:

- `⊞ Win + 1` -> switch everything to PC 1

- `⊞ Win + 2`-> switch everything to PC 2

- `⊞ Win + 3` -> switch everything to PC 3

This will:

✔ switch your **keyboard**<br>
✔ switch your **mouse**<br>
✔ switch your **monitor input** (optional)<br>

### Monitor switching can be turned on/off

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
   - input 17 => DisplayPort

4. Add these numbers to your config.ini

```ini
[SOURCES]
device1=5    ; HDMI-1
device2=6    ; HDMI-2
device3=17   ; DisplayPort
```

Save it. Restart the UnifiedSwitch process (or reboot).

Now `Win+1` / `Win+2` / `Win+3` will switch PC and screen perfectly and instantly.

![how ro restart process](images/image-1.png)<br>
-> press Win+1 / Win+2 / Win+3. Everything switches perfectly forever.

## Supported Connections

| Device                  | Connection type | Works with UnifiedSwitch?   |
| ----------------------- | --------------- | --------------------------- |
| MX Keys / MX Mechanical | **Bluetooth**   | ✔ Works perfectly           |
| MX Keys / MX Mechanical | Logi Bolt       | Not tested                  |
| MX Master 3 / 3S        | **Bluetooth**   | ✔ Works perfectly           |
| MX Master 3 / 3S        | Logi Bolt       | ❌ Does not switch channels |

> [!TIP]
> Use **direct Bluetooth pairing** on every PC (no dongle).  
> You get lower latency, longer battery life, and 100 % working channel switching with this script.

## huge thanks

- todbot/hidapitester
- nirsoft/ControlMyMonitor
