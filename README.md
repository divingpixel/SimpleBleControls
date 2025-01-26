# WHY THIS PROJECT

I needed a way to configure and interact with my microcontrollers without attaching buttons, screens and other devices, and I didn't liked the solutions that where already on the market.
I didn't want to install Home Assistant on a different computer, have a wifi router and other unrelated devices and I don't have the need to monitor and watch instant realtime changes.
I also have a number of devices (for example lights, central heating, etc) that I would like to adjust sometimes, without being forced to use very few buttons on a very limited interface.
It is like a remote control for devices that work in "fire and forget" mode, but that from time to time need adjustments. 

Since Bluetooth it's pretty ubiqutuous, all the mobile phones have it, and nowadays very cheap microcontrollers integrate this wireless technology, I decided to solve my problem this way.
The project is split in two parts : a library to easily configure the microcontroller to create the desired controls, and a mobile app that will display the contols that are received as BLE characteristics.

# HOW DOES THIS WORK

This is Arduino compatible code that can be used on an ESP32 based microcontroller to configure and generate BLE Characteristics.
You need to #include it in your project and below there is the documentation on how to use it.

You will also need the mobile phone app that will interpret the caracteristics and will display some "controls" according to the data in the characteristics.
In the mobile app the microcontroller can be configured and also data from the microcontroller can be observed.
The Android app can be found here ..., and the iPhone version will be also available soon.

# THE LIBRARY

Import the library and create an EspBleControls object pointer.

> *EspBleControls* controls;

In ``setup()`` initialise the pointer by creating a new EspBleControls object with two parameters.

> controls = new EspBleControls("Kitchen Controller", 228378);

First parameter is the name that will be displayed for the device, and second is the pin code for pairing. If the pin is set to 0, no pairing is needed.

Then add the controls that you need.
## For now possible controls are:

### Clock control
A control to set and observe the time from the microcontroller RTC (internal or external)
![Clock control](/media/clock.png "Clock control")

> controls->createClockControl("Clock Control", initialTime, 1);

### Switch control
Just a simple switch (ON/OFF)
![Switch control](/media/switch.png "Switch control")

### Momentary control
Just like the switch, but momentary
![Momentary button control](/media/momentary.png "Momentary button control")

### Slider control
A slider, to set an integer value between two limits. Sadly it's limited to a Short range.
![Slider control](/media/slider.png "Slider control")

### Integer control
A text input where an integer can be set. Again if the limits are set, only short() values can be sent, but without limits a 32bit int can be used.
![Integer control](/media/integer.png "Integer control")

### Float control
Just like the integer, with the same limitations, but for floats.
![Float control](/media/float.png "Float control")

### String control
A text input box, that can be limited to a certain number of chars (less than 512).
![String control](/media/string.png "String control")

### Angle control
A nice touch input that can set a value between 0..359, integer.
![Angle control](/media/angle.png "Angle control")

### Color control
A nice interface to generate a color and returns the RGB in hex format.
![Color control](/media/color.png "Color control")

### Interval control
A sofisticated interface to set on/off intervals in the 24 hours loop.
![Interval control](/media/interval.png "Interval control")

> controls->createIntervalControl("Interval controller", 288, 10, [](bool isOn) -> void { if (isOn) toggleLed("ON"); else toggleLed("OFF"); });

The methods also have a small documentation just in case you need it (you will, just hover over the method name).

After creating the controls that you need call the pointer's ``startService()``!

> controls->startService();

Finally, don't forget to add in the ``loop()`` a call to the ``loopCallbacks()``.

> controls->loopCallbacks();

The main.cpp is a good example how to use these controls.

Have fun!
