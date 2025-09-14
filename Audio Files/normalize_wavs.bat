@echo off
setlocal

set TARGET=-12

for %%f in (*8bit.wav) do (
    echo Processing %%f ...
    ffmpeg -i "%%f" -af loudnorm=I=%TARGET%:TP=-1.5:LRA=11:linear=true -ac 1 -ar 22050 -c:a pcm_u8 "normalized_%%~nf.wav"
)

echo All files normalized, downsampled, and saved as 8-bit mono WAV!
pause
