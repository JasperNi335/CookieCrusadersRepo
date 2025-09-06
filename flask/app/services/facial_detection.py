# face_service.py
from __future__ import annotations
import io
from typing import List, Tuple, Union
import numpy as np
import cv2

# Load OpenCV's built-in frontal face cascade once (fast & simple)
_CASCADE = cv2.CascadeClassifier(
    cv2.data.haarcascades + "haarcascade_frontalface_default.xml"
)

def _to_bgr_image(img_source: Union[bytes, io.BytesIO, np.ndarray, str]) -> np.ndarray:
    """
    Accepts:
      - raw bytes (e.g., uploaded file.read())
      - BytesIO
      - numpy ndarray (RGB or BGR)
      - filesystem path (str)
    Returns:
      - OpenCV image in BGR color format
    """
    if isinstance(img_source, np.ndarray):
        img = img_source
        # If it's likely RGB (common from PIL), convert to BGR
        if img.ndim == 3 and img.shape[2] == 3:
            # Heuristic: assume RGB; convert to BGR
            img = img[:, :, ::-1].copy()
        return img

    if isinstance(img_source, (bytes, io.BytesIO)):
        data = img_source if isinstance(img_source, bytes) else img_source.getvalue()
        arr = np.frombuffer(data, dtype=np.uint8)
        img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        if img is None:
            raise ValueError("Could not decode image bytes.")
        return img

    if isinstance(img_source, str):
        img = cv2.imread(img_source, cv2.IMREAD_COLOR)
        if img is None:
            raise ValueError(f"Could not read image from path: {img_source}")
        return img

    raise TypeError("Unsupported image source type.")

def detect_faces(
    img_source: Union[bytes, io.BytesIO, np.ndarray, str],
    scale_factor: float = 1.1,
    min_neighbors: int = 5,
    min_size: Tuple[int, int] = (60, 60),
) -> List[Tuple[int, int, int, int]]:
    """
    Returns a list of bounding boxes [ (x, y, w, h), ... ]
    Coordinates are in pixel space relative to the input image.
    """
    img_bgr = _to_bgr_image(img_source)
    gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)

    faces = _CASCADE.detectMultiScale(
        gray,
        scaleFactor=scale_factor,
        minNeighbors=min_neighbors,
        minSize=min_size,
        flags=cv2.CASCADE_SCALE_IMAGE,
    )

    return [(int(x), int(y), int(w), int(h)) for (x, y, w, h) in faces]

def draw_boxes(
    img_source: Union[bytes, io.BytesIO, np.ndarray, str],
    boxes: List[Tuple[int, int, int, int]],
    thickness: int = 2,
) -> bytes:
    """
    Draws rectangles on a copy of the image and returns PNG bytes.
    """
    img_bgr = _to_bgr_image(img_source).copy()
    for (x, y, w, h) in boxes:
        cv2.rectangle(img_bgr, (x, y), (x + w, y + h), (0, 255, 0), thickness)

    ok, buf = cv2.imencode(".png", img_bgr)
    if not ok:
        raise RuntimeError("Failed to encode annotated image.")
    return buf.tobytes()
