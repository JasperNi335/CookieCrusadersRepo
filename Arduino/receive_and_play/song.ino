

// Notes
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247

#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494

#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988

#define REST     0

// Melody (note, duration ms)
static int melody[] = {
  NOTE_G4, 500, NOTE_G4, 250, NOTE_FS4, 250, NOTE_G4, 250, NOTE_DS4, 250, REST, 250, NOTE_AS3, 250, 
  NOTE_G4, 250, NOTE_G4, 250, NOTE_G4, 250, NOTE_F4, 250, NOTE_DS4, 500, REST, 500, 
  NOTE_GS4, 500, NOTE_GS4, 250, NOTE_G4, 250, NOTE_GS4, 250, NOTE_F4, 250, REST, 250, NOTE_AS3, 250, 
  NOTE_GS4, 250, NOTE_GS4, 250, NOTE_GS4, 250, NOTE_G4, 250, NOTE_F4, 500, REST, 500, 
  NOTE_AS4, 500, NOTE_AS4, 250, NOTE_A4, 250, NOTE_AS4, 250, NOTE_G4, 250, REST, 250, NOTE_DS4, 250, 
  NOTE_GS4, 250, NOTE_GS4, 250, NOTE_GS4, 250, NOTE_AS4, 250, NOTE_C5, 250, REST, 250, NOTE_DS5, 500, 
  NOTE_AS4, 250, NOTE_AS4, 250, NOTE_AS4, 250, NOTE_AS4, 250, NOTE_AS4, 250, NOTE_AS4, 250, NOTE_GS4, 250, NOTE_F4, 250, 
  NOTE_DS4, 2000
};

static int melodyLength = sizeof(melody) / sizeof(melody[0]);

static void playNote(int freq, int duration) {
  if (freq == REST) {
    noTone(speakerPin);
    delay(duration);
    return;
  }
  tone(speakerPin, freq);
  delay(duration * 0.9);
  noTone(speakerPin);
  delay(duration * 0.1);
}

void playCookieSong() {
  for (int i = 0; i < melodyLength; i += 2) {
    int note = melody[i];
    int duration = melody[i + 1];
    playNote(note, duration);
  }
}
