import vosk
import wave
import json
import os
import urllib.request
import zipfile

import soundfile as sf
import numpy as np
import scipy.signal

def convert_to_mono_16k_pcm(input_file, output_file="converted.wav"):
    data, samplerate = sf.read(input_file, dtype="float32")

    # Convert to mono if stereo
    if data.ndim > 1:
        data = np.mean(data, axis=1)

    # Resample to 16 kHz if needed
    if samplerate != 16000:
        num_samples = int(len(data) * 16000 / samplerate)
        data = scipy.signal.resample(data, num_samples)
        samplerate = 16000

    # Scale to int16 PCM
    data = np.int16(data * 32767)

    # Save as proper WAV
    sf.write(output_file, data, samplerate, subtype="PCM_16")
    print(f"Converted file saved as {output_file}")
    return output_file


def process_audio_file(FILE_NAME):
    """ 
    Process a single audio file for speech recognition 
    
    This function expects the audio file to be in the "testing_audio" directory
    and to be named as {FILE_NAME}.wav. That might need to be changed if necessary

    The input .wav file should be mono, 16-bit PCM, 16kHz. If not, the function will
    attempt to convert it using the `convert_to_mono_16k_pcm` function, which requires
    more processing time, plus the `soundfile` and `scipy` libraries.

    This function will work with files of any length.
    """
    audio_file = os.path.join(os.path.dirname(__file__), "testing_audio", f"{FILE_NAME}.wav")
    final_text = []

    opened = False
    while not opened:
        try:
            print(f"Opening {audio_file} ...")
            with wave.open(audio_file, "rb") as wf:
                if wf.getnchannels() != 1 or wf.getsampwidth() != 2 or wf.getframerate() != 16000:
                    raise ValueError("Audio must be mono, 16-bit PCM, 16kHz.")

                # ------------------------
                # Step 4: Feed in smaller chunks
                # ------------------------
                frames_per_chunk = 4000  # ≈0.25 sec at 16kHz
                while True:
                    data = wf.readframes(frames_per_chunk)
                    if len(data) == 0:
                        break
                    if recognizer.AcceptWaveform(data):
                        result = json.loads(recognizer.Result())
                        if result.get("text"):
                            print("Final:", result["text"])
                            final_text.append(result["text"])
                    else:
                        partial = json.loads(recognizer.PartialResult())
                        if partial.get("partial"):
                            print("Partial:", partial["partial"])
        except Exception as e:
            print(f"Error opening/processing file: {e}")
            print("Attempting to convert to mono 16kHz PCM...")
            audio_file = convert_to_mono_16k_pcm(audio_file)
        else:
            opened = True


    # Get last final result
    final = json.loads(recognizer.FinalResult())
    if final.get("text"):
        print("Final:", final["text"])
        final_text.append(final["text"])

    print("\nComplete Transcription:")
    print(" ".join(final_text))

# ------------------------
# Step 1: Model setup
# ------------------------
MODEL_NAME = "vosk-model-en-us-0.22" # This is an accurate but very heavy model.
MODEL_URL = f"https://alphacephei.com/vosk/models/{MODEL_NAME}.zip"

# Download model if not present
if not os.path.exists(MODEL_NAME):
    print(f"Model '{MODEL_NAME}' not found. Downloading...")
    zip_path = MODEL_NAME + ".zip"
    urllib.request.urlretrieve(MODEL_URL, zip_path)
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(".")
    os.remove(zip_path)
    print("Model downloaded and extracted.")

# ------------------------
# Step 2: Load model
# ------------------------
model = vosk.Model(MODEL_NAME)
recognizer = vosk.KaldiRecognizer(model, 16000)  # 16000 Hz sample rate

# ------------------------
# Step 3: Open test WAV file
# ------------------------

# Run tests on multiple files
# In the real thing, change this to the actual file you receive from the Pico
FILE_NAMES = ["hello", "long_recording", "test_1s", "test_17s", "where_cookies_go", "oh_boy"]
for FILE_NAME in FILE_NAMES:
    process_audio_file(FILE_NAME)