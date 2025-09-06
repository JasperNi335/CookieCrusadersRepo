# app/routes/web.py
from flask import Blueprint, render_template

bp = Blueprint("web", __name__)

@bp.get("/")
def home():
    return render_template("index.html", title="Home")