# Simin AutoClicker

**v3.0 — Goated Edition**  
A fast, lightweight Windows auto clicker with a modern purple UI, live CPS counter, and animated interface. No installation required.

---

## Features

- **Adjustable click interval** — 10ms to 99,999ms with a spin control
- **Mouse button selection** — Left, Right, or Middle button
- **Click type** — Single or Double click
- **Click location** — Follow your cursor, or lock to a fixed screen position
- **Position picker** — Minimize the window and click anywhere on screen to capture exact X/Y coordinates
- **Click limit** — Optionally stop after a set number of clicks
- **Configurable hotkey** — Toggle start/stop with F6, F7, F8, or F9
- **Live CPS display** — Real-time clicks-per-second counter
- **Total click counter** — Running total shown at all times
- **Separate START / STOP buttons** — Clear visual state at a glance
- **Flicker-free animated UI** — Double-buffered rendering with pulsing glow effects

---

## Download

Download `AutoClicker.exe` from this repository. No installer, no DLLs, no dependencies — just run it.

> **Note:** Windows SmartScreen may show a warning on first launch since the executable is unsigned.  
> Click **More info → Run anyway** to proceed. This is normal for unsigned tools.

---

## Usage

1. **Run** `AutoClicker.exe`
2. Set your desired **interval** (milliseconds between clicks)
3. Choose your **mouse button** and **click type**
4. Select **click location** — follow cursor or fixed position
5. Optionally enable a **click limit**
6. Choose your **hotkey** (default: F6)
7. Press **START** or tap your hotkey to begin
8. Press **STOP** or tap the hotkey again to stop

---

## Building from Source

### Requirements

- Windows 10 or later
- Visual Studio 2019 or newer (with Desktop C++ workload)

### Steps

1. Open Visual Studio
2. **File → New → Project → Windows Desktop Application**
3. Delete the auto-generated source files
4. **Right-click Source Files → Add → Existing Item** → select `autoclicker.cpp`
5. Press **Ctrl+Shift+B** to build

The `.exe` will be output to `Debug\` or `Release\` depending on your build configuration.

### Alternative — MinGW / MSYS2

```bash
pacman -S mingw-w64-x86_64-gcc   # install once via MSYS2

g++ -O2 -std=c++17 -o AutoClicker.exe autoclicker.cpp \
  -lcomctl32 -lgdi32 -luser32 -mwindows \
  -static -static-libgcc -static-libstdc++
```

---

## Controls Reference

| Control | Description |
|---|---|
| Interval (ms) | Time between each click in milliseconds |
| Mouse Button | Which mouse button to click |
| Click Type | Single or double click per interval |
| Click Location | Follow cursor or click at fixed X/Y |
| Pick Pos | Capture a fixed position by clicking on screen |
| Enable click limit | Stop automatically after N clicks |
| Toggle Hotkey | Keyboard shortcut to start/stop (F6–F9) |

---

## Hotkeys

| Key | Action |
|---|---|
| F6 (default) | Toggle start / stop |
| F7 / F8 / F9 | Alternative hotkeys (selectable in app) |

---

## Technical Notes

- Built with **Win32 API** and **C++17** — no third-party libraries
- **Double-buffered rendering** — smooth, flicker-free animated UI at ~25 fps
- Uses `WH_MOUSE_LL` low-level mouse hook for position capture
- Click thread runs independently with 10ms sleep granularity
- Fully **static-linked** — single portable `.exe`, no runtime DLLs needed

---

## License

Free to use for personal use.
