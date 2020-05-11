# LoRa APRS Tracker

The LoRa APRS Tracker will work with very cheep hardware which you can buy from amazon, ebay or aliexpress.
Try it out and be part of the APRS network.

## Supported boards

You can use one of the Lora32 boards:

* TTGO T-Beam V0.7 (433MHz SX1278)
* TTGO T-Beam V1 (433MHz SX1278)

This boards cost around 30 Euros, they are very cheap but perfect for an LoRa iGate.
Keep in minde: you need a 433MHz version!

## Compiling

### How to compile

The best success is to use PlatformIO. Go to https://platformio.org/ and download the IDE. Just open the folder and you can compile the Firmware.

### Dependencies

* [LoRa](https://github.com/sandeepmistry/arduino-LoRa) by Sandeep Mistry
* [APRS-Decoder-Lib](https://github.com/peterus/APRS-Decoder-Lib) by Peter Buchegger
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) by Adafruit (with all dependecies)
* [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus) by Mikal Hart
* [AXP202X_Library](https://github.com/lewisxhe/AXP202X_Library) by Lewis He

## Configuration

Change your configuration in settings.h

## Future plans

The complete configuration should move to [IotWebConf](https://github.com/prampec/IotWebConf).

## Lora iGate

Look at my other project: a [LoRa iGate](https://github.com/peterus/LoRa_APRS_iGate)
