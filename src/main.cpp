#include <ESP32Time.h>
#include <EspBleControls.h>

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
BLECharacteristic* intervalChar;

ESP32Time espRtc;
uint32_t timeValue = 1729800000UL;

EspBleControls* controls;

void toggleLed(std::string value) {
  if (value == "ON") {
    digitalWrite(LIGHT_PIN, HIGH);
  } else {
    digitalWrite(LIGHT_PIN, LOW);
  }
};

void notifyClock(uint16_t delayMs, uint32_t timeValue) { //TODO this function should be a Publisher for the time values
  if (controls->isAuthorised() && controls->isConnected()) {
    timerChar->setValue(timeValue);
    timerChar->notify();
    delay(delayMs); // TODO replace the delay with the timeCheck
  }
}

void setup() {

  espRtc.setTime(timeValue);
  pinMode(LIGHT_PIN, OUTPUT);

  controls = new EspBleControls("Kitchen Dimmer", 228378);

  timerChar = controls->createClockControl("Clock Control", timeValue, true, [](uint32_t value) -> void { espRtc.setTime(value); });
  switchChar = controls->createSwitchControl("Switch Control", "OFF", false, [](std::string value) -> void { toggleLed(value); });
  momentChar = controls->createMomentaryControl("Momentary Control", "OFF", false, [](std::string value) -> void { toggleLed(value); });
  sliderChar = controls->createSliderControl("Slider controller", -255, 255, 32, 0, false, [](int32_t value) -> void { /* do something with this value */ });
  intChar = controls->createIntControl("Integer controller", -512, 512, 456, false, [](int32_t value) -> void { /* do something with this value */ });
  floatChar = controls->createFloatControl("Float controller", -255, 255, 123.45, false, [](float_t value) -> void { /* do something with this value */ });
  stringChar = controls->createStringControl("Text controller", 128, "Text", false, [](std::string value) -> void { /* do something with this value */ });
  angleChar = controls->createAngleControl("Angle controller", 55, false, false, [](uint32_t value) -> void { /* do something with this value */ });
  colorChar = controls->createColorControl("Color controller", "FFFF00", false, [](std::string value) -> void { /* do something with this value */ });
  intervalChar = controls->createIntervalControl("Interval controller", 288, false, [](std::vector<bool> value) -> void { /* do something with this value */ });

  controls->startService();
}

void loop() {
  notifyClock(1000, espRtc.getEpoch());
}
