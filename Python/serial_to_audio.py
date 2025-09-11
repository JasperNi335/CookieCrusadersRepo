import serial
import time

PORT = 'COM5'
BAUD = 115200
SAMPLE_RATE = 16000
RECORD_SEC = 1
SAMPLE_COUNT = SAMPLE_RATE * RECORD_SEC
HEADER_SIZE = 44
GAIN = 50  # adjust for audibility

def apply_gain_and_remove_dc(wav_bytes, gain=1.0):
    header = wav_bytes[:44]
    audio = wav_bytes[44:]
    n_samples = len(audio) // 2
    samples = [audio[i] | (audio[i+1] << 8) for i in range(0, len(audio), 2)]
    # convert to signed
    signed = [s - 32768 for s in samples]
    mean = int(sum(signed) / n_samples)
    out = bytearray(len(audio))
    for i, s in enumerate(signed):
        v = int((s - mean) * gain)
        if v > 32767: v = 32767
        if v < -32768: v = -32768
        u = v + 32768
        out[2*i] = u & 0xFF
        out[2*i+1] = (u >> 8) & 0xFF
    return header + out

def capture_wav():
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
        time.sleep(2)
        print("Waiting for WAV data...")

        file_index = 0
        while True:
            line = ser.readline().decode(errors='ignore').strip()
            if "Sending WAV over USB" in line:
                filename = f"test_{file_index}.wav"
                print(f"Receiving WAV data... saving to {filename}")
                wav_bytes = bytearray()

                total_bytes = HEADER_SIZE + SAMPLE_COUNT*2
                bytes_read = 0
                while bytes_read < total_bytes:
                    chunk = ser.read(min(1024, total_bytes - bytes_read))
                    if chunk:
                        wav_bytes.extend(chunk)
                        bytes_read += len(chunk)

                processed = apply_gain_and_remove_dc(wav_bytes, gain=GAIN)
                with open(filename, "wb") as f:
                    f.write(processed)

                print(f"WAV saved as {filename}")
                file_index += 1

    except KeyboardInterrupt:
        print("Interrupted by user.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial port closed.")

if __name__ == "__main__":
    capture_wav()
