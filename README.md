# Shelly-1PM
Firmware for the Shelly 1PM (https://shelly.cloud/knowledge-base/devices/shelly-1pm/)

This is a firmware for the Shelly Dimmer 2 (see https://shelly.cloud/knowledge-base/devices/shelly-dimmer-2/). This firmware has been developped with the following libraries:
- ESP8266TimerInterrupt 1.4.0: https://github.com/khoih-prog/ESP8266TimerInterrupt
- WifiManager 2.0.3-alpha: https://github.com/tzapu/WiFiManager
- pubsubclient: https://github.com/arendst/Tasmota/tree/development/lib/default/pubsubclient-2.8.13
- Arduino IDE 1.8.3.

The board "Generic ESP8266 Module" should be selected when generating the compiled binary.

This firmware can be installed by connecting the Shelly device to a PC with an USB-to-UART adapter and flashing the firmware with the esptools. The firmware can be also flashed through the OTA (Over The Air) programming. This is done by first installing Tasmota on the device using the mgos-to-tasmota software (https://github.com/yaourdt/mgos-to-tasmota). Once Tasmota has been installed to the Shelly device, the firmware can be uploaded using the following gzip file https://github.com/Mollayo/Shelly-1PM/raw/master/shelly1PM.ino.generic.bin.gz.
