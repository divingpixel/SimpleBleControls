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

In setup() initialise the pointer by creating a new EspBleControls object with two parameters.
First parameter is the name that will be displayed for the device, and second is the pin code for pairing. If the pin is set to 0, no pairing is needed.

Then add the controls that you need.
## For now possible controls are:
* Clock control : to set and observe the time from the microcontroller RTC (internal or external)
* Switch control : a switch (ON/OFF)
* Momentary control : a switch, but momentary
* Slider control : a slider, to set an integer value between two limits. Sadly it's limited to a Short range.
* Integer control : a text input where an integer can be set. Again if the limits are set, only Shorts can be sent, but without limits a 32bit int can be used.
* Float control : like the integer, with the same limitations, but for floats.
* String control : a text input box, that can be limited to a certain number of chars (less than 512).
* Angle control : a nice touch input that can set a value between 0..359, integer.
* Color control : again a nice interface to generate a color and returns the RGB in hex format.
* Interval control : sofisticated interface to set on/off intervals in the 24 hours loop.

The methods have a small documentation also, just in case you need it (you will).

Finally call the pointer's startService()!
The main.cpp is a good example how to use these controls.

Have fun!
