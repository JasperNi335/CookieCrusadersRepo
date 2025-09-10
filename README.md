# CookieCrusaders (Pico W · MicroPython)

Zero-fuss workflow for **Windows + VS Code**. One key to bootstrap, one key to flash, one key to deploy.

> Works on Python **3.10+**. No Thonny required (optional notes at the end).

---

## Repo layout

CookieCrusadersRepo/
├─ .venv/ # created by Bootstrap (not in git)
├─ device/
│ ├─ boot.py
│ ├─ config.py
│ ├─ audio.py
│ └─ main.py
├─ tools/
│ ├─ bootstrap.py                   # one-shot: create .venv, install deps, set VS Code interpreter
│ ├─ flash_uf2.ps1                  # flash MicroPython UF2 (via BOOTSEL or mpremote->bootloader)
│ ├─ sync_to_pico.ps1               # copy device/*.py → board and reset
│ ├─ clean_device.ps1               # remove device files from the board and reset
│ ├─ list_ports.py                  # list serial ports (helpful if multiple boards)
│ ├─ kws_cookie_over_serial.py      # host helper: send "BEEP", read frames, etc.
│ └─ requirements.txt
├─ .vscode/
│ ├─ tasks.json                     # VS Code tasks wired to the scripts
│ ├─ keybindings.json               # keyboard shortcuts
│ ├─ settings.json                  # points VS Code to .venv
│ └─ extensions.json                # workspace extension recommendations
├─ pyrightconfig.json               # Pylance/typing config for MicroPython
├─ README.md
├─ .gitignore
└─ <MicroPython_UF2_here>.uf2       # put the Pico/Pico W UF2 file here (ignored by git)

---

## Requirements

- Windows 10/11
- Python **3.10+**
- VS Code
- A MicroPython UF2 for **Raspberry Pi Pico / Pico W** (download once, save it in the repo root)

---

## Quick start (Windows)

1) **Open folder in VS Code** (the repo root).  
2) Press **`Ctrl+Alt+S` – “Bootstrap dev env”**  
   - Creates `.venv`, installs `mpremote`, `pyserial`, and type stubs.  
   - Sets VS Code’s interpreter to the repo’s `.venv`.  
3) **Put a MicroPython UF2** in the repo root (e.g. `RPI_PICO_W-<version>.uf2`).  
4) Press **`Ctrl+Alt+D` – “Pico: Flash + Deploy”**  
   - Flashes the UF2 (will prompt for BOOTSEL if needed).  
   - Copies `device/*.py` to the board and resets it.  
5) Talk/clap near the mic or use the host command below to trigger a beep.

---

## Daily workflow

- **Update firmware & code in one go:**  
  **`Ctrl+Alt+D`** → *Pico: Flash + Deploy* (firmware + latest `device/` files).
- **Only update code (fast inner loop):**  
  **`Ctrl+Alt+B`** → *Device: Sync to Pico* (copies `device/` files, resets board).
- **Clean the device:**  
  **`Ctrl+Alt+C`** → *Device: Clean & Reboot* (removes `boot.py`, `config.py`, `audio.py`, `main.py` from the board, resets).

---

## Keyboard shortcuts

- **`Ctrl+Alt+S`** → Bootstrap dev env  
- **`Ctrl+Alt+Shift+F`** → Firmware: Flash MicroPython UF2  
- **`Ctrl+Alt+B`** → Device: Sync to Pico  
- **`Ctrl+Alt+D`** → Pico: Flash + Deploy  
- **`Ctrl+Alt+C`** → Device: Clean & Reboot

(Also available via **Terminal → Run Task…**)

---

## Host utilities (optional)

With the `.venv` that Bootstrap created:

- **List COM ports**
  ```powershell
  .\.venv\Scripts\python.exe tools\list_ports.py