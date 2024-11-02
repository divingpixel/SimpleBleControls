# THE PROJECT

I needed a way to configure and interact with my microcontrollers without attaching buttons, screens and other devices, so I decided to use BLE to do this.
The project is split in two parts : a library to easily configure the microcontroller to create the desired controls, and a mobile app that will display the contols that are received as BLE characteristics.

# WHAT IS THIS

This is Arduino compatible code that can be used on an ESP32 based microcontroller to configure and generate BLE Characteristics.
Then there is and mobile phone app that will interpret these caracteristics and will display some touch controls according to the data in the characteristics.
This way, the microcontroller can be configured and also data from the microcontroller can be observed.
Currently the Android app is in development, but as soon as it's done and published, an IPhone version will be also developed.

# HOW IT WORKS

Import the library and create an EspBleControls pointer.

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
![Compass control](/media/compass.png "Compass control")

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
