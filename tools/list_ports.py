import sys
import serial.tools.list_ports as lp

for p in lp.comports():
    print(f"{p.device:>6}  {p.vid:04X}:{p.pid:04X}  {p.manufacturer or ''}  {p.description}")
print("\nPick the *data* CDC port (the other one is the REPL).")