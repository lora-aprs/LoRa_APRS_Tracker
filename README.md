# LoRa APRS Tracker 

Forked from <https://github.com/lora-aprs/LoRa_APRS_Tracker>, Peter, OE5BPA

The LoRa APRS Tracker will work with very cheep hardware which you can buy from amazon, ebay or aliexpress.
Try it out and be part of the APRS network.

![TTGO T-Beam](pics/Tracker.png)

## Supported boards

You can use one of the Lora32 boards:

* TTGO T-Beam V0.7 (433MHz SX1278)
* TTGO T-Beam V1 (433MHz SX1278)

This boards cost around 30 Euros, they are very cheap but perfect for an LoRa iGate.
Keep in minde: you need a 433MHz version!

## Intention

I forked this version because of my intention to build a specialized tracker for my mobile home. It should collect data from some sensors, weather condition etc. and send it to the APRS-System in two ways: If no wifi is present, it should just send the Packet to 433 MHz. If a known wifi is present, it should try to send the packet via wifi to the APRS-System direct.

### Dependencies

* [LoRa](https://github.com/sandeepmistry/arduino-LoRa) by Sandeep Mistry
* [APRS-Decoder-Lib](https://github.com/peterus/APRS-Decoder-Lib) by Peter Buchegger
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306) by Adafruit (with all dependecies)
* [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus) by Mikal Hart
* [AXP202X_Library](https://github.com/lewisxhe/AXP202X_Library) by Lewis He

## Configuration

Change your configuration in /data/is-cfg,json

## Future plans

At the source i forked the complete configuration may move to [IotWebConf](https://github.com/prampec/IotWebConf).
It may possible i move too...

* [ ] add sending packet direct via wifi to aprs-net (using work from Peter, OE5BPA)
* [ ] some refactoring
* [ ] REST-API or communication with webserver
* [ ] web-page sensors information
* [ ] etc.

# History

01.03.2021  Forked
05.03.2021  Moved configuration to json-file in filesystem
