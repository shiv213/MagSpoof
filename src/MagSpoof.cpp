#include "LittleFS.h"
#include <Arduino.h>
#include <Dictionary.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

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

String tracks[2];
Dictionary &d = *(new Dictionary());
String curr_id = "";

WiFiManager wm;
WiFiManagerParameter custom_field;

String getParam(String name) {
  String value;
  if (wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback() {
  Serial.println("[CALLBACK] saveParamCallback fired");
  String value = getParam("customfieldid");
  Serial.println("PARAM customfieldid = " + value);
  String card = d[value];
  tracks[0] = card.substring(0, card.indexOf(';')) + "\0";
  tracks[1] = card.substring(card.indexOf(';')) + "\0";
}

void setup() {
  Serial.begin(9600);
  Serial.setDebugOutput(true);

  LittleFS.begin();
  File db = LittleFS.open("/db.csv", "r");
  while (db.available()) {
    String temp = db.readStringUntil('\n');
    d(temp.substring(0, temp.indexOf(',')),
      temp.substring(temp.indexOf(',') + 1));
  }
  db.close();

  String card = d[curr_id];

  tracks[0] = card.substring(0, card.indexOf(';')) + "\0";
  tracks[1] = card.substring(card.indexOf(';')) + "\0";
  Serial.println(tracks[0]);
  Serial.println(tracks[1]);
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  WiFi.mode(WIFI_STA);
  int count = d.count();
  String custom_field_html = "<label for='customfieldid'>Select one of the "
                             "following: </label><br><br>";
  for (int i = 0; i < count; i++) {
    if (d[i] == curr_id) {
      custom_field_html += "<input checked type='radio' id='" + d(i) +
                           "' name='customfieldid' value='" + d(i) + "'>" +
                           d(i) + "<br>";
    } else {
      custom_field_html += "<input type='radio' id='" + d(i) +
                           "' name='customfieldid' value='" + d(i) + "'>" +
                           d(i) + "<br>";
    }
  }
  Serial.println(custom_field_html);
  // const char *custom_field_html =
  //     "<label for='label'>Label:</label><input type='text' "
  //     "name='label'><br><br><label for='tracks'>Tracks "
  //     ":</label><input type='text' name='tracks'><br><br>";
  char *temp = new char[custom_field_html.length() + 1];
  new (&custom_field)
      WiFiManagerParameter(strcpy(temp, custom_field_html.c_str()));

  wm.addParameter(&custom_field);
  wm.setConfigPortalBlocking(false);
  wm.setSaveParamsCallback(saveParamCallback);

  std::vector<const char *> menu = {"param", "info", "sep", "restart", "exit"};
  wm.setMenu(menu);

  wm.setClass("invert");
  wm.autoConnect("foopsgam", "magspoof");
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

void playTrack(int track) {
  int tmp, crc, lrc = 0;
  track--;
  polarity = 0;

  digitalWrite(ENABLE_PIN, HIGH);

  for (int i = 0; i < 25; i++)
    playBit(0);

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
  for (int i = 0; i < 25; i++)
    playBit(0);

  digitalWrite(PIN_A, LOW);
  digitalWrite(PIN_B, LOW);
  digitalWrite(ENABLE_PIN, LOW);
}

void loop() {
  wm.process();
  playTrack(1 + (curTrack++ % 2));
  delay(400);
}
