from app import create_app
from setup.install_models import install_voice_model

install_voice_model()
app = create_app()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)