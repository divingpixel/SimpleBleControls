#include <EspBleControls.h>
#include <ESP32Time.h>

#define LIGHT_PIN 21

// The initialTime should be initialised in setup() with the value retrieved from a rtc module if it exists
const uint32_t initialTime = 1730000000UL;
EspBleControls* controls;
Publisher<std::string> isLightOn;
Publisher<int32_t> intValue;

void setup() {

  pinMode(LIGHT_PIN, OUTPUT);
  isLightOn.doOnSet([](std::string value) -> void { digitalWrite(LIGHT_PIN, (value == "ON") ? HIGH : LOW); });

  controls = new EspBleControls("Kitchen Controller", 228378);

  controls->createClockControl("Clock Control", initialTime, 1, [](uint32_t value) -> void { });
  controls->createIntervalControl("Interval controller", 288, 10, [](bool isOn) -> void { isLightOn.setValue((isOn) ? "ON" : "OFF", nullptr); });
  controls->createSwitchControl("Switch Control", "OFF", &isLightOn, [](std::string value) -> void { });
  controls->createMomentaryControl("Momentary Control", "OFF", &isLightOn, [](std::string value) -> void { });
  controls->createSliderControl("Slider controller", -255, 255, 32, 0, &intValue, [](int32_t value) -> void { });
  controls->createIntControl("Integer controller", -512, 512, 0, &intValue, [](int32_t value) -> void { });
  controls->createFloatControl("Float controller", -255, 255, 123.45, nullptr, [](float_t value) -> void { });
  controls->createStringControl("Text controller", 128, "Text", nullptr, [](std::string value) -> void { });
  controls->createAngleControl("Angle controller", 55, false, nullptr, [](uint32_t value) -> void { });
  controls->createColorControl("Color controller", "FFFF00", nullptr, [](std::string value) -> void { });

  controls->startService();
}

void loop() {
  controls->updateControls();
}
