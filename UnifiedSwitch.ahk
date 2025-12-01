#NoTrayIcon
#SingleInstance Force
#MenuMaskKey vkFF
SetBatchLines, -1
SetWorkingDir %A_ScriptDir%

; Load settings with defaults
IniRead, multi, config.ini, SETTINGS, multiMonitor, 0
IniRead, mon, config.ini, PATHS, clickmon, ""
if (mon && !InStr(mon, ":\") && !InStr(mon, "\\"))
    mon := A_ScriptDir "\" mon

; Win+1/2/3: Switch to display input (0,1,2)
$#1::
$#2::
$#3::
    SendInput, {LWin up}
    KeyWait, LWin  ; Wait for physical release to clear Win key state
    Sleep 50
    
    n := SubStr(A_ThisHotkey, 0)
    param := n - 1  ; LogiSwitch.exe expects 0,1,2
    
    if (!multi) {  ; Use ControlMyMonitor for primary display
        IniRead, dev, config.ini, SOURCES, device%n%, ""
        if (dev && mon)
            Run, %ComSpec% /c ""%mon%" /SetValue Primary 60 %dev%",, Hide
    }
    
    RunWait, LogiSwitch.exe %param%,, Hide
    MouseMove, 1, 0, 0, R  ; Wake mouse
return

Esc::ExitApp