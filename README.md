# elecrow_iot
Add ESP8266-01 Wi-Fi module to the Elecrow plant watering system.

You'll need to solder a jumper wire between...
--RST pin on the serial header to A4 pin
--D7 pin to the bottom pin of the menu toggle button (the button just below the power button, and directly above the RTC battery slot)

In watering_kit.ino, you'll need to replace a few values.  I should have made them globals (and might eventually), however, was running low
on stack/heap space.

Search for the following items in watering_kit.ino and replace them accordingly

SERVER_IP_HERE : IP address of the host you run the nodejs express server on.
SERVER_PORT_HERE : Whatever port you configure the nodejs express server to run on
AP_NAME_HERE : Router broadcast name
AP_PASS_HERE : Router password
