################################################################
# Made by: MrDanS_21 and ChatGPT                               #
#                                                              #
# TODO: implement facial recognition and return left or right  #
#       in ../services/facial_detection.py                     #
################################################################

from flask import Blueprint, request, jsonify
from werkzeug.exceptions import BadRequest
from ..services.ingest import append_chunk
#from ..services.facial_detection import TODO

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
        return jsonify({"complete": False}), 200

    if not status["jpeg_valid"]:
        return jsonify({"complete": True, "error": "Invalid JPEG"}), 400

    # image is complete + valid
    #result = TODO(status["image_bytes"])
    #return jsonify(result), 200
    
    return 501