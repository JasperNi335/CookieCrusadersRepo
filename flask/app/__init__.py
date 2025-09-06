from flask import Flask
from .config import Config
from .routes.esp import bp as esp_bp
from .routes.web import bp as web_bp
from .routes.test import bp as test_bp

def create_app():
    app = Flask(__name__, static_folder="static", template_folder="templates")
    app.config.from_object(Config())

    # Blueprints
    app.register_blueprint(esp_bp, url_prefix="/esp")
    app.register_blueprint(web_bp)
    app.register_blueprint(test_bp, url_prefix="/test")

    return app