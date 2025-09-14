#include <Arduino.h>
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
#include "want_cookie.h"
#include "little_cookie.h"
#include "me_cookie_monster.h"
#include "secretly_elmo.h"
#include "choc_chip.h"
#include "veggies.h"
#include "what_the.h"
#include "cookie_monster.h"
#include "nomnom.h"

const int speakerPin = 27;  // PWM-capable pin for speaker
const int RX_PIN = 1;       // Pico W UART RX pin (connect to ESP TX)
const int TX_PIN = 0;       // Pico W UART TX pin (connect to ESP RX)

// Play an 8-bit PCM WAV stored in PROGMEM (.h array)
void playWav(const unsigned char *data, unsigned int length) {
  unsigned int i = 44;  // skip WAV header
  while (i < length) {
    uint8_t b = data[i++];
    analogWrite(speakerPin, b);  // PWM duty 0–255
    delayMicroseconds(125);      // 8 kHz playback
  }
}

// Pick a file based on the received command
void playCommand(const String &cmd) {
  if (cmd == "A") {
    Serial.println("Playing: where_cookies_go");
    playWav(where_cookies_go, where_cookies_go_len);
  } else if (cmd == "B") {
    Serial.println("Playing: oh_boy");
    playWav(oh_boy, oh_boy_len);
  } else if (cmd == "C") {
    Serial.println("Playing: c_is_for_cookie");
    playWav(c_is_for_cookie, c_is_for_cookie_len);
  } else if (cmd == "D") {
    Serial.println("Playing: gasp_cookie");
    playWav(gasp_cookie, gasp_cookie_len);
  } else if (cmd == "E") {
    Serial.println("Playing: arrr_nomnom");
    playWav(arrr_nomnom, arrr_nomnom_len);
  } else if (cmd == "F") {
    Serial.println("Playing: veggies");
    playWav(veggies, veggies_len);
  } else if (cmd == "G") {
    Serial.println("Playing: ah");
    playWav(ah, ah_len);
  } else if (cmd == "H") {
    Serial.println("Playing: cheers");
    playWav(cheers, cheers_len);
  } else if (cmd == "I") {
    Serial.println("Playing: choc_chip");
    playWav(choc_chip, choc_chip_len);
  } else if (cmd == "J") {
    Serial.println("Playing: cookie_monster");
    playWav(cookie_monster, cookie_monster_len);
  } else if (cmd == "K") {
    Serial.println("Playing: eat_cookies");
    playWav(eat_cookies, eat_cookies_len);
  } else if (cmd == "L") {
    Serial.println("Playing: hello");
    playWav(hello, hello_len);
  } else if (cmd == "M") {
    Serial.println("Playing: me_cookie_monster");
    playWav(me_cookie_monster, me_cookie_monster_len);
  } else if (cmd == "N") {
    Serial.println("Playing: milk");
    playWav(milk, milk_len);
  } else if (cmd == "O") {
    Serial.println("Playing: nomnom");
    playWav(nomnom, nomnom_len);
  } else if (cmd == "P") {
    Serial.println("Playing: secretly_elmo");
    playWav(secretly_elmo, secretly_elmo_len);
  } else if (cmd == "Q") {
    Serial.println("Playing: want_cookie");
    playWav(want_cookie, want_cookie_len);
  } else if (cmd == "R") {
    Serial.println("Playing: what_the");
    playWav(what_the, what_the_len);
  } else if (cmd == "S") {
    Serial.println("Playing: song");
    playCookieSong();
  } else if (cmd == "Z") {
    Serial.println("Playing: eat_children");
    playWav(eat_children, eat_children_len);
  } else {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
  }
}

void setup() {
  pinMode(speakerPin, OUTPUT);

  // Configure PWM
  analogWriteFreq(20000);    // push PWM out of audible range
  analogWriteResolution(8);  // 8-bit resolution

  // UART to ESP
  Serial1.setRX(RX_PIN);
  Serial1.setTX(TX_PIN);
  Serial1.begin(115200);

  // Debug over USB
  Serial.begin(115200);

  Serial.println("Pico W ready.");
  Serial1.println("Pico W ready.");  // handshake to ESP
}

void loop() {
  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');  // read from esp
    msg.trim();

    if (msg.length() > 0) {
      playCommand(msg);

      // Send back ACK to esp
      Serial1.print("ACK: ");
      Serial1.println(msg);

      // Also log to USB
      Serial.print("Received from ESP: ");
      Serial.println(msg);
    }
  }
}
