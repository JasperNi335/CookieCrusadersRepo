# tools/bootstrap.py
import os, sys, subprocess, venv, json

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VENV_DIR = os.path.join(REPO_ROOT, ".venv")
IS_WIN = os.name == "nt"
PY = os.path.join(VENV_DIR, "Scripts", "python.exe") if IS_WIN else os.path.join(VENV_DIR, "bin", "python")

def run(cmd):
    print(">>", " ".join(cmd))
    subprocess.check_call(cmd)

def ensure_venv():
    if not os.path.isdir(VENV_DIR):
        print("Creating venv at", VENV_DIR)
        venv.create(VENV_DIR, with_pip=True)
    else:
        print("venv already exists:", VENV_DIR)
    return PY

def install_deps(py):
    req = os.path.join(REPO_ROOT, "tools", "requirements.txt")
    run([py, "-m", "pip", "install", "--upgrade", "pip"])
    if os.path.exists(req):
        run([py, "-m", "pip", "install", "-r", req])
    else:
        # Fallback (shouldn't happen since you have requirements.txt)
        run([py, "-m", "pip", "install", "mpremote", "pyserial",
             "micropython-stdlib-stubs", "micropython-rp2-pico-stubs"])

def config_vscode(py):
    vscode_dir = os.path.join(REPO_ROOT, ".vscode")
    os.makedirs(vscode_dir, exist_ok=True)
    settings_path = os.path.join(vscode_dir, "settings.json")
    data = {}
    if os.path.exists(settings_path):
        try:
            with open(settings_path, "r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception:
            data = {}
    # Point VS Code to the repo venv
    data["python.defaultInterpreterPath"] = py
    with open(settings_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print("Updated VS Code settings:", settings_path)

def main():
    if sys.version_info < (3, 10):
        print("Python 3.10+ is required. Detected:", sys.version)
        sys.exit(1)
    py = ensure_venv()
    install_deps(py)
    config_vscode(py)
    print("\n✅ Bootstrap complete.")
    print("Next:")
    print(" - Place a MicroPython UF2 (Pico/Pico W) in repo root if needed.")
    print(" - Run: ‘Firmware: Flash MicroPython UF2’ then ‘Pico: Deploy’ from VS Code tasks.")

if __name__ == "__main__":
    main()