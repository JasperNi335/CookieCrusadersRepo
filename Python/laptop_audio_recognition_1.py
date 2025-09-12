import vosk
import sounddevice as sd
import queue
import json
import time
import numpy as np
import os
import urllib.request
import zipfile
from collections import deque

# ------------------------
# Step 1: Download model if missing
# ------------------------
MODEL_NAME = "vosk-model-en-us-0.22"
MODEL_URL = f"https://alphacephei.com/vosk/models/{MODEL_NAME}.zip"

if not os.path.exists(MODEL_NAME):
    print(f"Downloading {MODEL_NAME} ...")
    zip_path = MODEL_NAME + ".zip"
    urllib.request.urlretrieve(MODEL_URL, zip_path)
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(".")
    os.remove(zip_path)
    print("Model ready.")

# ------------------------
# Step 2: Initialize recognizer
# ------------------------
print("Initializing model...")
model = vosk.Model(MODEL_NAME)
recognizer = vosk.KaldiRecognizer(model, 16000)

# ------------------------
# Step 3: Audio queue
# ------------------------
q = queue.Queue()

def callback(indata, frames, time_info, status):
    q.put(bytes(indata))

print("🎙️ Speak (Ctrl+C to stop)")

# ------------------------
# Step 4: Settings
# ------------------------
SILENCE_DURATION = 1.2          # seconds of silence to trigger final
SMOOTH_BLOCKS = 10              # RMS smoothing over last N blocks
MIN_BLOCKS_BEFORE_FINAL = 3     # prevent very short finals
AMBIENT_WINDOW = 50             # sliding window for ambient noise RMS
MIN_THRESHOLD = 0.02            # minimum speech threshold

last_sound_time = time.time()
blocks_since_last_final = 0
rms_buffer = deque(maxlen=SMOOTH_BLOCKS)
ambient_rms_buffer = deque(maxlen=AMBIENT_WINDOW)

def safe_rms(block):
    """Compute RMS of audio block, normalized to [-1,1]."""
    if block.size == 0:
        return 0.0
    block = block.astype(np.float32) / 32768.0
    return np.sqrt(np.mean(block**2))

# ------------------------
# Step 5: Audio stream
# ------------------------
with sd.RawInputStream(samplerate=16000, blocksize=800, dtype="int16",
                       channels=1, callback=callback):
    while True:
        data = q.get()
        block = np.frombuffer(data, dtype=np.int16)

        # Compute normalized RMS
        rms = safe_rms(block)
        rms_buffer.append(rms)
        smooth_rms = np.mean(rms_buffer)

        # Update ambient RMS buffer
        ambient_rms_buffer.append(rms)
        ambient_rms = np.mean(ambient_rms_buffer)

        # Dynamic threshold based on ambient noise
        dynamic_threshold = max(MIN_THRESHOLD, ambient_rms * 1.5)

        blocks_since_last_final += 1

        # Feed all audio to recognizer
        recognizer.AcceptWaveform(data)

        # Reset silence timer if RMS exceeds dynamic threshold
        if smooth_rms > dynamic_threshold:
            last_sound_time = time.time()

        # Partial results for live feedback
        partial = json.loads(recognizer.PartialResult())
        if partial.get("partial"):
            print(f"\r⌛ Partial: {partial['partial']:<50}", end="")

        # Final result after silence, only if minimum blocks reached
        if (time.time() - last_sound_time > SILENCE_DURATION
                and blocks_since_last_final >= MIN_BLOCKS_BEFORE_FINAL):
            result = json.loads(recognizer.Result())
            if result.get("text"):
                print(f"\n✅ Final: {result['text']}")
            recognizer = vosk.KaldiRecognizer(model, 16000)
            last_sound_time = time.time()
            blocks_since_last_final = 0
            rms_buffer.clear()
