#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

void setPolarity(int polarity);
void playBit(int bit);

#define PIN_A 14
#define PIN_B 12
#define ENABLE_PIN 2
#define CLOCK_US 400
#define BETWEEN_ZERO 53

int polarity = 0;
unsigned int curTrack = 0;
char revTrack[80];

const int sublen[] = {32, 48, 48};
const int bitlen[] = {7, 5, 5};

const char* tracks[] = {"INSERT CARD TRACK 1 \0", "INSERT CARD TRACK 2 \0"};


void storeRevTrack(int track) {
  int i, tmp, crc, lrc = 0;
  track--;
  polarity = 0;

  for (i = 0; tracks[track][i] != '\0'; i++) {
    crc = 1;
    tmp = tracks[track][i] - sublen[track];

    for (int j = 0; j < bitlen[track] - 1; j++) {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      tmp & 1 ? (revTrack[i] |= 1 << j) : (revTrack[i] &= ~(1 << j));
      tmp >>= 1;
    }
    crc ? (revTrack[i] |= 1 << 4) : (revTrack[i] &= ~(1 << 4));
  }

  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track] - 1; j++) {
    crc ^= tmp & 1;
    tmp & 1 ? (revTrack[i] |= 1 << j) : (revTrack[i] &= ~(1 << j));
    tmp >>= 1;
  }
  crc ? (revTrack[i] |= 1 << 4) : (revTrack[i] &= ~(1 << 4));

  i++;
  revTrack[i] = '\0';
}

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  storeRevTrack(2);
  Serial.begin(9600);
}

void setPolarity(int polarity) {
  if (polarity) {
    digitalWrite(PIN_B, LOW);
    digitalWrite(PIN_A, HIGH);
  } else {
    digitalWrite(PIN_A, LOW);
    digitalWrite(PIN_B, HIGH);
  }
}

void playBit(int bit) {
  polarity ^= 1;
  setPolarity(polarity);
  delayMicroseconds(CLOCK_US);

  if (bit == 1) {
    polarity ^= 1;
    setPolarity(polarity);
  }

  delayMicroseconds(CLOCK_US);
}

void reverseTrack(int track) {
  int i = 0;
  track--;
  polarity = 0;

  while (revTrack[i++] != '\0')
    ;
  i--;
  while (i--)
    for (int j = bitlen[track] - 1; j >= 0; j--)
      playBit((revTrack[i] >> j) & 1);
}

void playTrack(int track) {
  int tmp, crc, lrc = 0;
  track--;
  polarity = 0;

  digitalWrite(ENABLE_PIN, HIGH);

  for (int i = 0; i < 25; i++) playBit(0);

  for (int i = 0; tracks[track][i] != '\0'; i++) {
    crc = 1;
    tmp = tracks[track][i] - sublen[track];

    for (int j = 0; j < bitlen[track] - 1; j++) {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      playBit(tmp & 1);
      tmp >>= 1;
    }
    playBit(crc);
  }

  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track] - 1; j++) {
    crc ^= tmp & 1;
    playBit(tmp & 1);
    tmp >>= 1;
  }
  playBit(crc);
  for (int i = 0; i < 25; i++) playBit(0);

  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
  digitalWrite(ENABLE_PIN, LOW);
}

void loop() {
  Serial.println("Working");
  memset(revTrack, 1, sizeof(revTrack));
  playTrack(1 + (curTrack++ % 2));
  delay(400);
}
