import os
import wave
import json
from typing import List

import numpy as np
import soundfile as sf
import scipy.signal
import vosk

# Model availiable at https://alphacephei.com/vosk/models/vosk-model-en-us-0.22.zip
MODEL_PATH = os.path.join(
    os.path.dirname(__file__), "..", "models", "vosk-model-en-us-0.22"
)

def convert_to_mono_16k_pcm(input_file: str, output_file: str = "converted.wav") -> str:
    """
    Convert an audio file to mono, 16 kHz, 16-bit PCM WAV format.

    Args:
        input_file: Path to the input audio file.
        output_file: Path where the converted file will be saved.

    Returns:
        The path to the converted WAV file.
    """
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
    return output_file


def process_audio_file(audio_file: str) -> str:
    """
    Transcribe speech from a single audio file.

    Args:
        audio_file: Path to the input WAV file (mono, 16-bit PCM, 16 kHz preferred).
                    If the file does not meet these requirements, it will be converted.

    Returns:
        The transcribed text as a string.
    """
    model = vosk.Model(MODEL_PATH)
    recognizer = vosk.KaldiRecognizer(model, 16000)

    final_text: List[str] = []
    opened = False

    while not opened:
        try:
            with wave.open(audio_file, "rb") as wf:
                if (
                    wf.getnchannels() != 1
                    or wf.getsampwidth() != 2
                    or wf.getframerate() != 16000
                ):
                    raise ValueError("Audio must be mono, 16-bit PCM, 16kHz.")

                frames_per_chunk = 4000  # ≈0.25 sec at 16kHz
                while True:
                    data = wf.readframes(frames_per_chunk)
                    if len(data) == 0:
                        break
                    if recognizer.AcceptWaveform(data):
                        result = json.loads(recognizer.Result())
                        if result.get("text"):
                            final_text.append(result["text"])
        except Exception:
            # Attempt to convert and retry
            audio_file = convert_to_mono_16k_pcm(audio_file)
        else:
            opened = True

    # Add the final result
    final = json.loads(recognizer.FinalResult())
    if final.get("text"):
        final_text.append(final["text"])

    return " ".join(final_text).strip()
