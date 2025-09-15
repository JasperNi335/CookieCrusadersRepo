################################################################
# Made by: MrDanS_21 and ChatGPT                               #
#                                                              #
# using DNN facial detection for best accuracy                 #
# model dependancies:                                          #
#   ../models/deploy.prototxt                                  #
#   ../models/res10_300x300_ssd_iter_140000.caffemodel         #
# @param conf_threshold: adjust for sensitivity                #
################################################################

from __future__ import annotations
import io, os, time
from typing import List, Tuple, Union, Optional, Iterable
import numpy as np
import cv2

_DNN = None
def _load_dnn():
    global _DNN
    if _DNN is None:
        base = os.path.join(os.path.dirname(__file__), "..", "models")
        prototxt = os.path.abspath(os.path.join(base, "deploy.prototxt"))
        weights  = os.path.abspath(os.path.join(base, "res10_300x300_ssd_iter_140000.caffemodel"))
        _DNN = cv2.dnn.readNetFromCaffe(prototxt, weights)
        # CPU is fine; these calls are harmless if the backend/target isn't available
        _DNN.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
        _DNN.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)
    return _DNN

def detect_faces(
    img_source: Union[bytes, io.BytesIO, np.ndarray, str],
    conf_threshold: float = 0.4
) -> List[Tuple[int,int,int,int]]:
    img_bgr = _to_bgr_image(img_source)
    net = _load_dnn()
    (h, w) = img_bgr.shape[:2]
    blob = cv2.dnn.blobFromImage(img_bgr, 1.0, (300, 300), (104.0, 177.0, 123.0))
    net.setInput(blob)
    detections = net.forward()

    boxes: List[Tuple[int,int,int,int]] = []
    for i in range(detections.shape[2]):
        confidence = float(detections[0, 0, i, 2])
        if confidence >= conf_threshold:
            box = detections[0, 0, i, 3:7] * np.array([w, h, w, h])
            (x1, y1, x2, y2) = box.astype("int")
            x, y = max(0, x1), max(0, y1)
            x2, y2 = min(w-1, x2), min(h-1, y2)
            boxes.append((x, y, x2 - x, y2 - y))
    return boxes

def _to_bgr_image(img_source: Union[bytes, io.BytesIO, np.ndarray, str]) -> np.ndarray:
    if isinstance(img_source, np.ndarray):
        # Assume input is BGR already; if you pass RGB (e.g., from PIL), convert before calling.
        return img_source
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

def draw_boxes(img_source, boxes: List[Tuple[int,int,int,int]], thickness: int = 2) -> bytes:
    img_bgr = _to_bgr_image(img_source).copy()
    for (x, y, w, h) in boxes:
        cv2.rectangle(img_bgr, (x, y), (x + w, y + h), (0, 255, 0), thickness)
    ok, buf = cv2.imencode(".png", img_bgr)
    if not ok:
        raise RuntimeError("Failed to encode annotated image.")
    return buf.tobytes()

def _box_area(box: Tuple[int,int,int,int]) -> int:
    """Return area of (x, y, w, h)."""
    x, y, w, h = box
    # Guard negatives just in case
    return max(0, w) * max(0, h)

def closest_box(boxes: Iterable[Tuple[int,int,int,int]]) -> Optional[Tuple[int,int,int,int]]:
    """
    Heuristic: the 'closest person' is the detection with the LARGEST bounding box area.
    Ties are broken by taller height, then by left-most x.
    Returns None if no boxes.
    """
    boxes = list(boxes)
    if not boxes:
        return None
    # max by (area, height, -x) so bigger area wins, then taller, then lower x (i.e., more left) if still tied
    return max(boxes, key=lambda b: (_box_area(b), b[3], -b[0]))

def box_center(box: Tuple[int,int,int,int]) -> Tuple[int,int]:
    """Return integer pixel coords (cx, cy) for the center of the box."""
    x, y, w, h = box
    cx = x + w // 2
    cy = y + h // 2
    return (cx, cy)

def box_region(box: Tuple[int,int,int,int], image_width: int) -> str:
    """
    Return 'L', 'M', or 'R' depending on which horizontal zone the BOX CENTER falls into.
    Zones:
      - 'L': left 2/5 of the image
      - 'M': middle 1/5 of the image
      - 'R': right 2/5 of the image
    """
    cx, _ = box_center(box)
    left_cut = 0.4 * image_width
    right_cut = 0.6 * image_width

    if cx < left_cut:
        return 'R'
    elif cx < right_cut:
        return 'M'
    else:
        return 'L'

def classify_closest_person(
    img_source: Union[bytes, io.BytesIO, np.ndarray, str],
    conf_threshold: float = 0.4
) -> Tuple[bytes, str]:
    """
    Detect faces, pick the 'closest' (largest box), classify its position into
    'L'/'M'/'R' by horizontal thirds, and return an annotated image too.

    Returns:
        (annotated_png_bytes, side)
        - annotated_png_bytes: PNG bytes with boxes drawn
        - side: 'L' | 'M' | 'R' (defaults to 'M' if no faces)
    """
    # Decode and detect
    img_bgr = _to_bgr_image(img_source)
    h, w = img_bgr.shape[:2]
    boxes = detect_faces(img_bgr, conf_threshold=conf_threshold)

    # Decide side: default 'M' if none
    side = 'M'
    if boxes:
        cb = closest_box(boxes)
        side = box_region(cb, w)

    # Always produce annotated image
    annotated = draw_boxes(img_bgr, boxes)
    return annotated, side
