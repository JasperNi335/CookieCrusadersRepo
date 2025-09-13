import os
import urllib
import zipfile

MODELS_PATH = os.path.join(os.path.dirname(__file__), "..", "app", "models")

VOICE_MODEL_NAME = "vosk-model-en-us-0.22"
VOICE_MODEL_URL = f"https://alphacephei.com/vosk/models/{VOICE_MODEL_NAME}.zip"
VOICE_MODEL_PATH = os.path.join(MODELS_PATH, VOICE_MODEL_NAME)

def install_voice_model():
    if not os.path.exists(VOICE_MODEL_PATH):
        print(f"Downloading {VOICE_MODEL_NAME} ...")
        zip_path = os.path.join(MODELS_PATH, VOICE_MODEL_NAME + ".zip")
        urllib.request.urlretrieve(VOICE_MODEL_URL, zip_path)
        with zipfile.ZipFile(zip_path, "r") as zip_ref:
            zip_ref.extractall(MODELS_PATH)
        os.remove(zip_path)
        print(f"Voice model: '{VOICE_MODEL_NAME}' installed.")
    else:
        print(f"Voice model: '{VOICE_MODEL_NAME}' already installed.")