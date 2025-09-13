import vosk
import sounddevice as sd
import queue
import json
import time
import numpy as np
import os
from collections import deque
import threading

MODEL_PATH = os.path.join(os.path.dirname(__file__), "..", "models", "vosk-model-en-us-0.22")

class SpeechRecognizer:
    def __init__(self, voice_map_path):
        print("Initializing VOSK model...")
        self.model = vosk.Model(MODEL_PATH)
        self.voice_map = self._load_voice_map(voice_map_path)

        self.recognizer = vosk.KaldiRecognizer(self.model, 16000)
        self.q = queue.Queue()

        # thresholds and buffers
        self.SILENCE_DURATION = 1.2
        self.SMOOTH_BLOCKS = 10
        self.MIN_BLOCKS_BEFORE_FINAL = 3
        self.AMBIENT_WINDOW = 50
        self.MIN_THRESHOLD = 0.02

        self.last_sound_time = time.time()
        self.blocks_since_last_final = 0
        self.rms_buffer = deque(maxlen=self.SMOOTH_BLOCKS)
        self.ambient_rms_buffer = deque(maxlen=self.AMBIENT_WINDOW)

        # storage for last recognized mapped phrase
        self._last_match = None
        self._lock = threading.Lock()

        self._stop_event = threading.Event()
        self._thread = None

    def _load_voice_map(self, path):
        with open(path) as f:
            return json.load(f)

    def _safe_rms(self, block):
        if block.size == 0:
            return 0.0
        block = block.astype(np.float32) / 32768.0
        return np.sqrt(np.mean(block ** 2))

    def _callback(self, indata, frames, time_info, status):
        self.q.put(bytes(indata))

    def _process(self):
        with sd.RawInputStream(samplerate=16000, blocksize=800, dtype="int16",
                               channels=1, callback=self._callback):
            while not self._stop_event.is_set():
                try:
                    data = self.q.get(timeout=0.1)
                except queue.Empty:
                    continue

                block = np.frombuffer(data, dtype=np.int16)
                rms = self._safe_rms(block)
                self.rms_buffer.append(rms)
                smooth_rms = np.mean(self.rms_buffer)

                self.ambient_rms_buffer.append(rms)
                ambient_rms = np.mean(self.ambient_rms_buffer)
                dynamic_threshold = max(self.MIN_THRESHOLD, ambient_rms * 1.5)

                self.blocks_since_last_final += 1
                self.recognizer.AcceptWaveform(data)

                if smooth_rms > dynamic_threshold:
                    self.last_sound_time = time.time()

                if (time.time() - self.last_sound_time > self.SILENCE_DURATION
                        and self.blocks_since_last_final >= self.MIN_BLOCKS_BEFORE_FINAL):
                    result = json.loads(self.recognizer.Result())
                    if result.get("text"):
                        text = result["text"].lower()
                        print(f"✅ Final: {text}")
                        self._handle_result(text)
                    self.recognizer = vosk.KaldiRecognizer(self.model, 16000)
                    self.last_sound_time = time.time()
                    self.blocks_since_last_final = 0
                    self.rms_buffer.clear()

    def _handle_result(self, text):
        if text in self.voice_map:
            entry = self.voice_map[text]
            mapped = f"{entry['letter']}{entry['duration']}"
            with self._lock:
                self._last_match = mapped

    def get_last_match(self):
        with self._lock:
            match = self._last_match
            self._last_match = None   # clear after read
        return match

    def start(self):
        if self._thread and self._thread.is_alive():
            return
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._process, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_event.set()
        if self._thread:
            self._thread.join()

_recognizer = None

def get_listener(voice_map_path: str) -> SpeechRecognizer:
    """
    Lazily create and start a single SpeechRecognizer instance.
    Safe to call repeatedly; returns the same instance each time.
    """
    global _recognizer
    if _recognizer is None:
        _recognizer = SpeechRecognizer(voice_map_path)
        _recognizer.start()
    return _recognizer