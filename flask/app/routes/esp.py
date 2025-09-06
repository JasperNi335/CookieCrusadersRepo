from flask import Blueprint

bp = Blueprint("api", __name__)

@bp.get("/stream")
def stream():
    pass