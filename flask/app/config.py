import os
from pathlib import Path

class Config:
    SECRET_KEY = os.getenv("SECRET_KEY", "devsecret")
    API_KEY = os.getenv("API_KEY", "changeme123")

    # File storage
    DATA_DIR = Path(os.getenv("DATA_DIR", "data")).resolve()
    ROTATE_DAILY = True          # one file per day
    MAX_FILE_MB = 50             # safety cut-off per file