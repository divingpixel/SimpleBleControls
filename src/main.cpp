#include <EspBleControls.h>

#define LIGHT_RED_PIN 2
#define LIGHT_GREEN_PIN 3
#define LIGHT_BLUE_PIN 4

// The initialTime should be initialised in setup() with the value retrieved from a rtc module if it exists
const uint32_t initialTime = 1730000000UL;
EspBleControlsFactory* controlsFactory;
ControlPublisher<std::string> isLightOn;
ControlPublisher<int32_t> intValue;
std::string color = "FFFFFF";

void toggleLight() {
  uint8_t red = stoi(color.substr(0,2), 0, 16);
  uint8_t green = stoi(color.substr(2,2), 0, 16);
  uint8_t blue = stoi(color.substr(4,2), 0, 16);
  if ((isLightOn.getValue() == "ON") ? HIGH : LOW) {
    analogWrite(LIGHT_RED_PIN, red); 
    analogWrite(LIGHT_GREEN_PIN, green);
    analogWrite(LIGHT_BLUE_PIN, blue); 
  } else {
    analogWrite(LIGHT_RED_PIN, 0);
    analogWrite(LIGHT_GREEN_PIN, 0);
    analogWrite(LIGHT_BLUE_PIN, 0); 
  }
}

void setup() {

  pinMode(LIGHT_RED_PIN, ANALOG);
  pinMode(LIGHT_GREEN_PIN, ANALOG);
  pinMode(LIGHT_BLUE_PIN, ANALOG);

  isLightOn.doOnSet([](std::string value) -> void { toggleLight (); });

  controlsFactory = new EspBleControlsFactory("Kitchen Controller", 228378);
  controlsFactory->createClockControl("Clock Control", initialTime, 1, [](uint32_t value) -> void { });
  controlsFactory->createIntervalControl("Interval controller", 288, 10, [](bool isOn) -> void { isLightOn.setValue((isOn) ? "ON" : "OFF", nullptr); });
  controlsFactory->createSwitchControl("Switch Control", "OFF", &isLightOn, [](std::string value) -> void { });
  controlsFactory->createMomentaryControl("Momentary Control", "OFF", &isLightOn, [](std::string value) -> void { });
  controlsFactory->createSliderControl("Slider controller", -255, 255, 32, 0, &intValue, [](int32_t value) -> void { });
  controlsFactory->createIntControl("Integer controller", -512, 512, 0, &intValue, [](int32_t value) -> void { });
  controlsFactory->createFloatControl("Float controller", -255, 255, 123.45, nullptr, [](float_t value) -> void { });
  controlsFactory->createStringControl("Text controller", 128, "Text", nullptr, [](std::string value) -> void { });
  controlsFactory->createAngleControl("Angle controller", 55, false, nullptr, [](uint32_t value) -> void { });
  controlsFactory->createColorControl("Color controller", "FFFF00", nullptr, [](std::string value) -> void { color = value; toggleLight(); });

  controlsFactory->startService();
}

void loop() {
  controlsFactory->updateControls();
}
