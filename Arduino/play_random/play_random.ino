#include <Arduino.h>
#include <Arduino.h>
#include <stdlib.h> // for rand()

// Include your WAV header files
#include "arrr_nomnom.h"   // WAV data array from flash
#include "c_is_for_cookie.h"
#include "eat_children.h"
#include "gasp_cookie.h"
#include "kys.h"
#include "oh_boy.h"
#include "where_cookies_go.h"
#include "ah.h"
#include "cheers.h"
#include "eat_cookies.h"
#include "milk.h"
#include "hello.h"
#include "want-cookie.h"
#include "little_cookie.h"
#include "me_cookie_monster.h"
#include "secretly_elmo.h"
#include "choc_chip.h"
#include "veggies.h"
#include "what_the.h"

const int speakerPin = 27; // PWM-capable pin

// Array of WAV files (pointers to arrays and their lengths)
struct WavFile {
  const uint8_t *data;
  size_t length;
};

WavFile wavFiles[] = {
  {arrr_nomnom_8bit_wav, arrr_nomnom_8bit_wav_len},
  {c_is_for_cookie_8bit_wav, c_is_for_cookie_8bit_wav_len},
  {eat_children_8bit_wav, eat_children_8bit_wav_len},
  {gasp_cookie_8bit_wav, gasp_cookie_8bit_wav_len},
  {kys_8bit_wav, kys_8bit_wav_len},
  {oh_boy_8bit_wav, oh_boy_8bit_wav_len},
  {where_cookies_go_8bit_wav, where_cookies_go_8bit_wav_len},
  {ah_8bit_wav, ah_8bit_wav_len},
  {cheers_8bit_wav, cheers_8bit_wav_len},
  {eat_cookies_8bit_wav, eat_cookies_8bit_wav_len},
  {milk_8bit_wav, milk_8bit_wav_len},
  {hello_8bit_wav, hello_8bit_wav_len},
  {want_cookie_8bit_wav, want_cookie_8bit_wav_len},
  {little_cookie_8bit_wav, little_cookie_8bit_wav_len},
  {me_cookie_monster_8bit_wav, me_cookie_monster_8bit_wav_len},
  {secretly_elmo_8bit_wav, secretly_elmo_8bit_wav_len},
  {choc_chip_8bit_wav, choc_chip_8bit_wav_len},
  {veggies_8bit_wav, veggies_8bit_wav_len},
  {what_the_8bit_wav, what_the_8bit_wav_len},
  // Add more WAVs here
};

const int numWavs = sizeof(wavFiles) / sizeof(wavFiles[0]);

void playWav(const uint8_t *data, size_t length) {
  // Skip WAV header
  size_t i = 44;
  while (i < length) {
    uint8_t b = data[i++];
    analogWrite(speakerPin, b); // write 0–255
    delayMicroseconds(125);      // 8 kHz playback
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(speakerPin, OUTPUT);

  // High-frequency PWM for cleaner audio
  analogWriteFreq(20000);
  analogWriteResolution(8);

  // Seed random number generator
  randomSeed(analogRead(0));
}

void loop() {
  // Pick a random WAV
  int idx = random(numWavs);
  Serial.print("Playing WAV #");
  Serial.println(idx);

  playWav(wavFiles[idx].data, wavFiles[idx].length);

  // Wait random interval (5–36 seconds)
  int delaySeconds = random(5, 7);
  Serial.print("Waiting for ");
  Serial.print(delaySeconds);
  Serial.println(" seconds...");
  delay(delaySeconds * 1000);
}
