# elecrow\_iot
Add ESP8266-01 Wi-Fi module to the Elecrow plant watering system.

## Hardware Modifications
You'll need to solder a jumper wire between...
* __RST__ pin on the serial header to __A4__ pin
* __D7__ pin to the bottom pin of the menu toggle button (the button just below the power button, and directly above the RTC battery slot - solder to the pin closest to the RTC battery slot)

## Configuration Changes
In watering\_kit.ino, you'll need to replace a few values.  I should have made them globals (and might eventually), however, was running low on stack/heap space.

Search for the following items in watering\_kit.ino and replace them accordingly

* __SERVER\_IP\_HERE__ : IP address of the host you run the nodejs express server on.
* __SERVER\_PORT\_HERE__ : Whatever port you configure the nodejs express server to run on
* __AP\_NAME\_HERE__ : Router broadcast name
* __AP\_PASS\_HERE__ : Router password
