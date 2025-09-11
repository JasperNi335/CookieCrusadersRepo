#include <Arduino.h>

#define AUDIO_ADC_PIN 26
#define SAMPLE_RATE 16000
#define RECORD_SEC 1
#define CHUNK_SIZE 512  // bytes per serial write

#define MODE_WAV   0  // raw WAV output
#define MODE_PLOT  1  // ASCII for Serial Plotter

uint8_t mode = MODE_WAV;  // change to MODE_WAV when sending WAVs

// Take multiple ADC readings and average
uint16_t readMic() {
    int sum = 0;
    const int N = 4;  // average 4 readings
    for(int i=0;i<N;i++) sum += analogRead(AUDIO_ADC_PIN);
    return sum / N;
}

void write_u32le(uint32_t n) {
  Serial.write(n & 0xFF);
  Serial.write((n >> 8) & 0xFF);
  Serial.write((n >> 16) & 0xFF);
  Serial.write((n >> 24) & 0xFF);
}

void write_u16le(uint16_t n) {
  Serial.write(n & 0xFF);
  Serial.write((n >> 8) & 0xFF);
}

void send_wav_header(uint32_t data_size) {
  Serial.write("RIFF");
  write_u32le(36 + data_size);
  Serial.write("WAVEfmt ");
  write_u32le(16);       // PCM fmt chunk
  write_u16le(1);        // PCM
  write_u16le(1);        // mono
  write_u32le(SAMPLE_RATE);
  write_u32le(SAMPLE_RATE * 2);  // byte rate (16-bit)
  write_u16le(2);        // block align
  write_u16le(16);       // bits per sample
  Serial.write("data");
  write_u32le(data_size);
}

// buf: pointer to byte array of length numSamples*2
// numSamples: number of 16-bit samples in the buffer
// gain: software gain multiplier
void processPCMFrame(uint8_t* buf, size_t numSamples, float gain=2.0) {
    if (numSamples == 0) return;

    // --- Step 1: compute mean ---
    uint32_t sum = 0;
    for (size_t i = 0; i < numSamples; i++) {
        uint16_t sample = buf[2*i] | (buf[2*i+1] << 8);
        sum += sample;
    }
    uint16_t mean = sum / numSamples;

    // --- Step 2: subtract mean and apply gain ---
    for (size_t i = 0; i < numSamples; i++) {
        uint16_t sample = buf[2*i] | (buf[2*i+1] << 8);
        int32_t centered = (int32_t)sample - mean;
        int32_t amplified = (int32_t)(centered * gain);

        // Clip to signed 16-bit
        if (amplified > 32767) amplified = 32767;
        if (amplified < -32768) amplified = -32768;

        uint16_t outSample = (uint16_t)(amplified + 32768);

        // write back little-endian
        buf[2*i]   = outSample & 0xFF;
        buf[2*i+1] = (outSample >> 8) & 0xFF;
    }
}

// buf = pointer to 16-bit samples stored little-endian (uint8_t* or uint16_t* castable)
// numSamples = number of 16-bit samples
// targetPeak = maximum signed magnitude you want (<= 32767)
void normalizePCMTo16bit(uint8_t* buf, size_t numSamples, int targetPeak = 30000) {
    if (numSamples == 0) return;

    // Step 1: find current signed extremes (convert from u16->int32 centered)
    int32_t maxAbs = 0;
    for (size_t i = 0; i < numSamples; ++i) {
        uint16_t u = buf[2*i] | (buf[2*i+1] << 8);
        int32_t s = (int32_t)u - 32768;
        int32_t a = s < 0 ? -s : s;
        if (a > maxAbs) maxAbs = a;
    }

    if (maxAbs == 0) return; // silent buffer

    // Step 2: compute scale factor (float)
    float scale = (float)targetPeak / (float)maxAbs;

    // Step 3: apply scaling and write back (clamp to int16)
    for (size_t i = 0; i < numSamples; ++i) {
        uint16_t u = buf[2*i] | (buf[2*i+1] << 8);
        int32_t s = (int32_t)u - 32768;
        int32_t scaled = (int32_t)roundf(s * scale);
        if (scaled > 32767) scaled = 32767;
        if (scaled < -32768) scaled = -32768;
        uint16_t out = (uint16_t)(scaled + 32768);
        buf[2*i]   = out & 0xFF;
        buf[2*i+1] = (out >> 8) & 0xFF;
    }
}


void setup() {
  Serial.begin(115200);
  while(!Serial);
  pinMode(AUDIO_ADC_PIN, INPUT);
  Serial.println("Pico ready");
}

void loop() {

  const unsigned long period_us = 1000000 / SAMPLE_RATE;
  unsigned long t_next = micros();
  const uint32_t samples = SAMPLE_RATE * RECORD_SEC;

  if (mode == MODE_PLOT) {
      // Plotting mode: send ASCII for Serial Plotter
    for (uint32_t i = 0; i < samples; i++) {
        uint16_t val = readMic();
        Serial.println(val);       // one value per line
        t_next += period_us;
        while ((long)(t_next - micros()) > 0);
    }
    delay(500); // small pause between frames
  } 
  else if (mode == MODE_WAV) {

    Serial.println("Sending WAV over USB");

    uint32_t samples = SAMPLE_RATE * RECORD_SEC;
    uint16_t *buffer = (uint16_t*)malloc(samples * sizeof(uint16_t));
    if(!buffer) {
      Serial.println("RAM allocation failed!");
      delay(1000);
      return;
    }

    unsigned long t_next = micros();
    const unsigned long period_us = 1000000 / SAMPLE_RATE;

    // --- Step 1: sample audio with oversampling/averaging ---
    const int OVERSAMPLE = 8;  // average 8 readings per sample
    for(uint32_t i=0; i<samples; i++) {
        uint32_t sum = 0;
        for(int j=0; j<OVERSAMPLE; j++) sum += analogRead(AUDIO_ADC_PIN);
        buffer[i] = sum / OVERSAMPLE;

        t_next += period_us;
        while ((long)(t_next - micros()) > 0); // wait for next sample
    }

    // --- Step 2: software DC removal + gain ---
    // after you've filled buffer and (optionally) processed DC/gain:
    uint8_t* byteBuf = (uint8_t*)buffer;

    // Option 1: do DC removal + gain then normalize
    processPCMFrame(byteBuf, samples, 3.0);   // optional
    normalizePCMTo16bit(byteBuf, samples, 30000);

    // --- Step 3: optional simple low-pass filter ---
    // y[n] = y[n-1]*alpha + x[n]*(1-alpha)
    float alpha = 0.2;  // smoothing factor: lower = smoother
    uint16_t prev = buffer[0];
    for(uint32_t i=1; i<samples; i++) {
        buffer[i] = prev * alpha + buffer[i] * (1-alpha);
        prev = buffer[i];
    }

    // --- Step 4: send WAV header + data ---
    send_wav_header(samples * 2);

    for(uint32_t i=0; i<samples; i+=CHUNK_SIZE/2) {
      uint32_t remaining = samples - i;
      uint32_t to_send = (remaining > CHUNK_SIZE/2) ? CHUNK_SIZE/2 : remaining;
      for(uint32_t j=0; j<to_send; j++) {
        Serial.write(buffer[i+j] & 0xFF);
        Serial.write((buffer[i+j] >> 8) & 0xFF);
      }
      delay(1);
    }

    free(buffer);
    delay(500);
  }
}
