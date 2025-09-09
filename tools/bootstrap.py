import os, subprocess, venv, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
VENV = ROOT / ".venv"

def py_in_venv():
    return VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")

def run(py, *args):
    print(">", py, *args)
    subprocess.check_call([str(py), *args])

def main():
    if not VENV.exists():
        print(f"Creating venv at {VENV} ...")
        venv.EnvBuilder(with_pip=True).create(VENV)
    py = py_in_venv()
    run(py, "-m", "pip", "install", "--upgrade", "pip")
    run(py, "-m", "pip", "install", "-r", str(ROOT / "tools" / "requirements.txt"))
    print("\n✅ Bootstrap done. In VS Code, select interpreter: .venv/Scripts/python.exe")

if __name__ == "__main__":
    main()