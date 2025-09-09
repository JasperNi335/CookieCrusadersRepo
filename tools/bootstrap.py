import os, sys, subprocess, venv, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
VENV = ROOT / ".venv"

def run(py, *args):
    print(">", py, *args)
    subprocess.check_call([str(py), *args])

def main():
    # 1) Create venv if missing
    if not VENV.exists():
        print(f"Creating venv at {VENV} ...")
        venv.EnvBuilder(with_pip=True).create(VENV)

    # 2) Pick venv python (works on Win/Mac/Linux)
    py = VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")

    # 3) Upgrade pip & install deps (no activation needed)
    run(py, "-m", "pip", "install", "--upgrade", "pip")
    req = ROOT / "tools" / "requirements.txt"
    run(py, "-m", "pip", "install", "-r", str(req))

    print("\n✅ Done!")
    print("Next in VS Code: Python: Select Interpreter → .venv/Scripts/python.exe (Win) or .venv/bin/python (Mac/Linux)")
    print("Then: Run Task → Device: Sync to Pico, Host: List COM ports, Host: Send BEEP")

if __name__ == "__main__":
    main()