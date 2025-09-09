# boot.py — runs at power-up
# Enable a second USB-CDC interface so we can stream binary data
# separately from the REPL console.
try:
    import usb_cdc
    # console=True keeps REPL on COMx, data=True creates a 2nd COMx for binary I/O
    usb_cdc.enable(console=True, data=True)
except Exception as e:
    # If your firmware doesn't have usb_cdc, comment this out and we'll fall back to sys.stdout.
    pass