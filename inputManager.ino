#include <ESP32RotaryEncoder.h>

#define ENC_CLK 25
#define ENC_DT 26
#define ENC_BTN 27

bool turnedRight = false;
bool turnedLeft = false;
bool buttonPressedShort = false;
bool buttonPressedLong = false;
RotaryEncoder rotaryEncoder(ENC_CLK, ENC_DT, ENC_BTN);

void knobCallback(long value) {
  if (turnedRight || turnedLeft)
    return;

  switch (value) {
    case 1:
      turnedRight = true;
      break;
    case -1:
      turnedLeft = true;
      break;
  }
  rotaryEncoder.setEncoderValue(0);
}

void buttonCallback(unsigned long duration) {
  if (duration < 500) {
    buttonPressedShort = true;
  } else {
    buttonPressedLong = true;
  }
}

void setupRotaryEncoder() {
  rotaryEncoder.setEncoderType(EncoderType::HAS_PULLUP);
  rotaryEncoder.setBoundaries(-1, 1, false);
  rotaryEncoder.onTurned(&knobCallback);
  rotaryEncoder.onPressed(&buttonCallback);
  rotaryEncoder.begin();
}

