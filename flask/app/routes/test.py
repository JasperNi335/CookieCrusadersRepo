from flask import Blueprint, send_file
import io, os
from ..services.facial_detection import detect_faces, draw_boxes

bp = Blueprint("test", __name__)

@bp.get("/facial")
def facial():
    # Hard-coded path to your test image
    img_path = os.path.abspath("Test/Data/3p.jpg")

    # Detect + annotate
    boxes = detect_faces(img_path)
    annotated = draw_boxes(img_path, boxes)

    # Send as PNG to the browser
    return send_file(
        io.BytesIO(annotated),
        mimetype="image/jpeg",
        as_attachment=False,
        download_name="faces.jpg"
    )