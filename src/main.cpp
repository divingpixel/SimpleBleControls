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

  controlsFactory->createClockControl("Contoller Clock", initialTime, 1, [](uint32_t value) -> void { externalRTC.set(value); });
  controlsFactory->createIntervalControl("Lights on Timer", 5, 5, [](bool isOn) -> void { isLightOn.setValue((isOn) ? "ON" : "OFF", nullptr); });
  controlsFactory->createSwitchControl("Light Switch", "OFF", &isLightOn, [](std::string value) -> void { });
  controlsFactory->createMomentaryControl("Momentary Light Switch", "OFF", false, &isLightOn, [](std::string value) -> void { });
  controlsFactory->createColorControl("Light Color", "FF0000", nullptr, [](std::string value) -> void { color = value; toggleLight(); });
  controlsFactory->createSliderControl("Integer Slider", -255, 255, 32, 0, &intValue, [](int32_t value) -> void { });
  controlsFactory->createIntControl("Integer Input", -512, 512, 0, &intValue, [](int32_t value) -> void { });
  controlsFactory->createFloatControl("Float Input", -255, 255, 123.45, nullptr, [](float_t value) -> void { });
  controlsFactory->createStringControl("Text Input", 128, "Text", nullptr, [](std::string value) -> void { });
  controlsFactory->createAngleControl("Angle Adjustment", 55, false, nullptr, [](uint32_t value) -> void { });

  controlsFactory->startService();
}

void loop() {
  controlsFactory->updateControls();
}
