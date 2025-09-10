################################################################
# Made by: MrDanS_21 and ChatGPT                               #
################################################################

from flask import Blueprint, request, jsonify
from werkzeug.exceptions import BadRequest
from ..services.ingest import append_chunk
from ..services.facial_detection import classify_closest_person
import os, time

bp = Blueprint("api", __name__)

@bp.get("/stream")
def stream():
    pass

@bp.post("/ingest")
def ingest():
    # accept either multipart (field "chunk") or raw body
    chunk = request.files["chunk"].read() if "chunk" in request.files else request.get_data()
    if not chunk:
        raise BadRequest("Empty chunk.")

    status = append_chunk(chunk)

    if not status["complete"]:
        # still accumulating bytes
        print("Recieved chunk, image incomplete")
        return jsonify({"complete": False}), 200

    if not status["jpeg_valid"]:
        print("Error: Invalid JPEG")
        return jsonify({"complete": True, "error": "Invalid JPEG"}), 400

    # image is complete + valid
    print("Recieved chunk, image complete")
    start = time.time()
    
    image = status["image_bytes"]
    annotated, side = classify_closest_person(image)
    
    end = time.time()
    print(f"Facial recognition ran in {(end - start)*1000:.6f} ms")
    
    with open(os.path.join(os.path.dirname(__file__), "..", "static", "tmp", "output.jpg"), "wb") as f:
        f.write(annotated)
    
    return side, 200