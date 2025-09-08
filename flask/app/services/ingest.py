################################################################
# Made by: MrDanS_21 and ChatGPT                               #
#                                                              #
# ts only works with jpg btw                                   #
################################################################

from pathlib import Path
from typing import Dict, Any

BASE_DIR = Path(__file__).resolve().parent
TMP_DIR = (BASE_DIR / ".." / "static" / "tmp").resolve()
TMP_FILE = TMP_DIR / "current.jpg.part"

SOI = b"\xFF\xD8"  # Start Of Image
EOI = b"\xFF\xD9"  # End Of Image

def _ensure_tmp() -> None:
    TMP_DIR.mkdir(parents=True, exist_ok=True)

def _file_size(p: Path) -> int:
    try:
        return p.stat().st_size
    except FileNotFoundError:
        return 0

def _ends_with_eoi(p: Path) -> bool:
    if not p.exists() or _file_size(p) < 2:
        return False
    with open(p, "rb") as f:
        f.seek(-2, 2)
        return f.read(2) == EOI

def _trim_to_last_eoi(data: bytes) -> bytes:
    pos = data.rfind(EOI)
    return data[:pos+2] if pos != -1 else data

def _jpeg_valid(data: bytes) -> bool:
    return len(data) >= 4 and data[:2] == SOI and data[-2:] == EOI

def append_chunk(chunk: bytes) -> Dict[str, Any]:
    """
    Append 'chunk' to a single temp file:
      - If chunk starts with SOI and a partial exists, auto-reset (treat as new image).
      - Finalize automatically when file ends with EOI.
      - On finalize: return the raw JPEG bytes (no saving), then delete the temp file.

    Returns:
      { "complete": False, "bytes": <partial_size> }
      or
      { "complete": True, "jpeg_valid": <bool>, "image_bytes": <bytes> }
    """
    _ensure_tmp()

    # Auto-reset if a new image clearly begins (chunk starts with SOI)
    if chunk.startswith(SOI) and TMP_FILE.exists() and _file_size(TMP_FILE) > 0:
        TMP_FILE.unlink(missing_ok=True)

    # Append
    with open(TMP_FILE, "ab") as f:
        f.write(chunk)

    # Not finished yet?
    if not _ends_with_eoi(TMP_FILE):
        return {"complete": False, "bytes": _file_size(TMP_FILE)}

    # Finalize: read, trim to last EOI (tolerate any trailing noise), validate, cleanup
    data = TMP_FILE.read_bytes()
    if not data.endswith(EOI):
        data = _trim_to_last_eoi(data)

    is_jpeg = _jpeg_valid(data)
    TMP_FILE.unlink(missing_ok=True)

    return {"complete": True, "jpeg_valid": is_jpeg, "image_bytes": data if is_jpeg else None}