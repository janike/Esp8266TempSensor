# MQTT temperature sensor for ESP8266
* PCB has esp8266, temperature sensor (DHT22), reset button and user button
* normal mode - wakes up, connects to wifi, reads temperature + humidity, publishes json data to MQTT broker and goes to sleep
* config mode - if button is pressed during boot, it goes to WebServer mode (release after blink - blinks 3 times); if button is pressed even longer (2 seconds after blink), it goes into AP mode and creates own wifi (blinks 4 times).
Wifi credentials, AP, Webserver mode, MQTT server, MQTT topic and SleepTime are configurable in config mode.
* Uses libraries: DHT, EEPROM, ArduinoJson, NTPClient, PubSubClient