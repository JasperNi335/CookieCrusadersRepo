# Lists serial ports (helps pick the Pico)
import serial.tools.list_ports as lp

for p in lp.comports():
    vid = f"{p.vid:04X}" if p.vid is not None else "----"
    pid = f"{p.pid:04X}" if p.pid is not None else "----"
    print(f"{p.device:>6}  {vid}:{pid}  {p.manufacturer or ''}  {p.description or ''}")