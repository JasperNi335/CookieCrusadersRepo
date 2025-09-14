# wav2h_batch.py
# Converts all WAV files in a folder to C header files with byte array representation.

import sys
import os
from pathlib import Path

def wav_to_header(input_file: Path):
    """Convert a .wav file to a C header file."""
    output_file = input_file.with_suffix(".h")

    with open(input_file, "rb") as f:
        data = f.read()

    array_name = input_file.stem.replace('.', '_')

    with open(output_file, "w") as f:
        f.write(f"const unsigned char {array_name}[] = {{\n")
        for i, b in enumerate(data):
            f.write(f"0x{b:02x},")
            if (i + 1) % 12 == 0:
                f.write("\n")
        f.write("\n};\n")
        f.write(f"const unsigned int {array_name}_len = {len(data)};\n")

    print(f"Converted: {input_file} -> {output_file}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python wav2h.py <folder_with_wavs>")
        sys.exit(1)

    folder = Path(sys.argv[1])

    if not folder.is_dir():
        print(f"Error: {folder} is not a valid folder.")
        sys.exit(1)

    wav_files = list(folder.glob("*.wav"))

    if not wav_files:
        print("No .wav files found in the folder.")
        return

    for wav_file in wav_files:
        wav_to_header(wav_file)

if __name__ == "__main__":
    main()
