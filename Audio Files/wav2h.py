# wav2h.py
# Converts a WAV file to a C header file with byte array representation.
# Thx gahost for providing the original script.

import sys

if len(sys.argv) < 2:
    print("Usage: python wav2h.py input.wav")
    sys.exit(1)

input_file = sys.argv[1]
output_file = input_file.rsplit('.', 1)[0] + ".h"

with open(input_file, "rb") as f:
    data = f.read()

with open(output_file, "w") as f:
    f.write(f"const unsigned char {input_file.replace('.', '_')}[] = {{\n")
    for i, b in enumerate(data):
        f.write(f"0x{b:02x},")
        if (i + 1) % 12 == 0:
            f.write("\n")
    f.write("\n};\n")
    f.write(f"const unsigned int {input_file.replace('.', '_')}_len = {len(data)};\n")