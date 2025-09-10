################################################################
# Made by: MrDanS_21 and ChatGPT                               #
#                                                              #
# use DNN facial detection for best accuracy                   #
# model dependancies:                                          #
#   ../models/deploy.prototxt                                  #
#   ../models/res10_300x300_ssd_iter_140000.caffemodel         #
# @param conf_threshold: adjust for sensitivity                #
################################################################

from __future__ import annotations
import io, os, time
from typing import List, Tuple, Union
import numpy as np
import cv2

# -------------------------
# Haar (kept for comparison)
# -------------------------
_HAAR = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")

def detect_faces_haar(img_bgr: np.ndarray) -> List[Tuple[int,int,int,int]]:
    gray = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2GRAY)
    gray = cv2.equalizeHist(gray)               # helps low-contrast 480p
    gray = cv2.GaussianBlur(gray, (3,3), 0)     # denoise a touch
    faces = _HAAR.detectMultiScale(gray, scaleFactor=1.06, minNeighbors=3, minSize=(30,30))
    return [(int(x), int(y), int(w), int(h)) for (x,y,w,h) in faces]

# -------------------------
# DNN (SSD ResNet-10)
# -------------------------
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

def detect_faces_dnn(
    img_bgr: np.ndarray,
    conf_threshold: float = 0.4
) -> List[Tuple[int,int,int,int]]:
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

# --------------- utilities ---------------
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

def detect_faces(img_source, method: str = "dnn") -> List[Tuple[int,int,int,int]]:
    img_bgr = _to_bgr_image(img_source)
    if method == "haar":
        return detect_faces_haar(img_bgr)
    return detect_faces_dnn(img_bgr)

def draw_boxes(img_source, boxes: List[Tuple[int,int,int,int]], thickness: int = 2) -> bytes:
    img_bgr = _to_bgr_image(img_source).copy()
    for (x, y, w, h) in boxes:
        cv2.rectangle(img_bgr, (x, y), (x + w, y + h), (0, 255, 0), thickness)
    ok, buf = cv2.imencode(".png", img_bgr)
    if not ok:
        raise RuntimeError("Failed to encode annotated image.")
    return buf.tobytes()
