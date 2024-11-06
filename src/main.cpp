#include <EspBleControls.h>
#include <DS3232RTC.h>

// Demo using an ESP32 C3 SuperMini board with an DS3231 RTC module and a RGB led
#define LIGHT_RED_PIN 2
#define LIGHT_GREEN_PIN 3
#define LIGHT_BLUE_PIN 4

// The initialTime should be initialised in setup() with the value retrieved from a rtc module if it exists
uint32_t initialTime = 1730000000UL;
DS3232RTC externalRTC;
EspBleControlsFactory* controlsFactory;
ControlPublisher<std::string> isLightOn;
ControlPublisher<int32_t> intValue;
std::string color = "FFFFFF";

void toggleLight() {
  if (isLightOn.getValue() == "ON") {
    analogWrite(LIGHT_RED_PIN, stoi(color.substr(0,2), 0, 16)); 
    analogWrite(LIGHT_GREEN_PIN, stoi(color.substr(2,2), 0, 16));
    analogWrite(LIGHT_BLUE_PIN, stoi(color.substr(4,2), 0, 16)); 
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
  
  externalRTC.begin();
  if (externalRTC.get() != 0) initialTime = externalRTC.get();

  isLightOn.doOnSet([](std::string value) -> void { toggleLight (); });

  controlsFactory = new EspBleControlsFactory("Kitchen Controller", 228378);

  controlsFactory->createClockControl("Clock Control", initialTime, 1, [](uint32_t value) -> void { externalRTC.set(value); });
  controlsFactory->createIntervalControl("Interval controller", 288, 10, [](bool isOn) -> void { isLightOn.setValue((isOn) ? "ON" : "OFF", nullptr); });
  controlsFactory->createSwitchControl("Switch Control", "OFF", &isLightOn, [](std::string value) -> void { });
  controlsFactory->createMomentaryControl("Momentary Control", "OFF", &isLightOn, [](std::string value) -> void { });
  controlsFactory->createColorControl("Color controller", "FF0000", nullptr, [](std::string value) -> void { color = value; toggleLight(); });
  controlsFactory->createSliderControl("Slider controller", -255, 255, 32, 0, &intValue, [](int32_t value) -> void { });
  controlsFactory->createIntControl("Integer controller", -512, 512, 0, &intValue, [](int32_t value) -> void { });
  controlsFactory->createFloatControl("Float controller", -255, 255, 123.45, nullptr, [](float_t value) -> void { });
  controlsFactory->createStringControl("Text controller", 128, "Text", nullptr, [](std::string value) -> void { });
  controlsFactory->createAngleControl("Angle controller", 55, false, nullptr, [](uint32_t value) -> void { });

  controlsFactory->startService();
}

void loop() {
  controlsFactory->updateControls();
}
