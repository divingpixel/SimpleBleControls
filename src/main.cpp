#include <EspBleControls.h>
#include <ESP32Time.h>

#define LIGHT_PIN 21

BLECharacteristic* timerChar;
BLECharacteristic* switchChar;
BLECharacteristic* sliderChar;
BLECharacteristic* stringChar;
BLECharacteristic* intChar;
BLECharacteristic* floatChar;
BLECharacteristic* angleChar;
BLECharacteristic* momentChar;
BLECharacteristic* colorChar;

const uint32_t initialTime = 1729800000UL;
uint32_t clockTimeStamp;
ESP32Time espClock;
EspBleControls* controls;

void toggleLed(std::string value) {
  if (value == "ON") {
    digitalWrite(LIGHT_PIN, HIGH);
  } else {
    digitalWrite(LIGHT_PIN, LOW);
  }
};

void notifyClock(uint16_t delaySeconds) { //TODO this function should be a Publisher for the time values
  if (hasTimePassed(clockTimeStamp, delaySeconds)) {
    //if (timerChar != nullptr && controls->isAuthorised() && controls->isConnected()) {
      uint32_t timeValue = espClock.getEpoch();
      timerChar->setValue(timeValue);
      timerChar->notify();
    //}
    clockTimeStamp = millis();
  }
}

void setup() {

  espClock.setTime(initialTime);
  pinMode(LIGHT_PIN, OUTPUT);

  controls = new EspBleControls("Kitchen Dimmer", 228378);

  timerChar = controls->createClockControl("Clock Control", initialTime, true, [](uint32_t time) -> void { espClock.setTime(time); });
  switchChar = controls->createSwitchControl("Switch Control", "OFF", false, [](std::string value) -> void { toggleLed(value); });
  momentChar = controls->createMomentaryControl("Momentary Control", "OFF", false, [](std::string value) -> void { toggleLed(value); });
  sliderChar = controls->createSliderControl("Slider controller", -255, 255, 32, 0, false, [](int32_t value) -> void { /* do something with this value */ });
  intChar = controls->createIntControl("Integer controller", -512, 512, 456, false, [](int32_t value) -> void { /* do something with this value */ });
  floatChar = controls->createFloatControl("Float controller", -255, 255, 123.45, false, [](float_t value) -> void { /* do something with this value */ });
  stringChar = controls->createStringControl("Text controller", 128, "Text", false, [](std::string value) -> void { /* do something with this value */ });
  angleChar = controls->createAngleControl("Angle controller", 55, false, false, [](uint32_t value) -> void { /* do something with this value */ });
  colorChar = controls->createColorControl("Color controller", "FFFF00", false, [](std::string value) -> void { /* do something with this value */ });
  controls->createIntervalControl("Interval controller", 288, 10, [](bool isOn) -> void { if (isOn) toggleLed("ON"); else toggleLed("OFF"); });

  controls->startService();
}

void loop() {
  notifyClock(1);
  controls->loopCallbacks();
}
