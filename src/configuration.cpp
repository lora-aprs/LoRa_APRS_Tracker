#include <SPIFFS.h>
#include <logger.h>

#ifndef CPPCHECK
#include <ArduinoJson.h>
#endif

#include "configuration.h"

ConfigurationManagement::ConfigurationManagement(String FilePath) : mFilePath(FilePath) {
  if (!SPIFFS.begin(true)) {
    logPrintlnE("Mounting SPIFFS was not possible. Trying to format SPIFFS...");
    SPIFFS.format();
    if (!SPIFFS.begin()) {
      logPrintlnE("Formating SPIFFS was not okay!");
    }
  }
}

// cppcheck-suppress unusedFunction
Configuration ConfigurationManagement::readConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    logPrintlnE("Failed to open file for reading...");
    return Configuration();
  }
  DynamicJsonDocument  data(2048);
  DeserializationError error = deserializeJson(data, file);

  if (error) {
    logPrintlnE("Failed to read file, using default configuration.");
  }
  file.close();

  Configuration conf;

  conf.debug = data["debug"] | false;

  JsonArray beacons = data["beacons"].as<JsonArray>();
  for (JsonVariant v : beacons) {
    Configuration::Beacon beacon;

    if (v.containsKey("callsign"))
      beacon.callsign = v["callsign"].as<String>();
    if (v.containsKey("path"))
      beacon.path = v["path"].as<String>();
    if (v.containsKey("message"))
      beacon.message = v["message"].as<String>();
    beacon.timeout = v["timeout"] | 1;
    if (v.containsKey("symbol"))
      beacon.symbol = v["symbol"].as<String>();
    if (v.containsKey("overlay"))
      beacon.overlay = v["overlay"].as<String>();
    if (v.containsKey("type"))
      beacon.type = v["type"].as<String>();

    beacon.smart_beacon.active      = v["smart_beacon"]["active"] | false;
    beacon.smart_beacon.turn_min    = v["smart_beacon"]["turn_min"] | 25;
    beacon.smart_beacon.slow_rate   = v["smart_beacon"]["slow_rate"] | 300;
    beacon.smart_beacon.slow_speed  = v["smart_beacon"]["slow_speed"] | 10;
    beacon.smart_beacon.fast_rate   = v["smart_beacon"]["fast_rate"] | 60;
    beacon.smart_beacon.fast_speed  = v["smart_beacon"]["fast_speed"] | 100;
    beacon.smart_beacon.min_tx_dist = v["smart_beacon"]["min_tx_dist"] | 100;
    beacon.smart_beacon.min_bcn     = v["smart_beacon"]["min_bcn"] | 5;

    beacon.enhance_precision = v["enhance_precision"] | false;

    conf.beacons.push_back(beacon);
  }

  conf.button.tx          = data["button"]["tx"] | false;
  conf.button.alt_message = data["button"]["alt_message"] | false;

  conf.lora.frequencyRx     = data["lora"]["frequency_rx"] | 433775000;
  conf.lora.frequencyTx     = data["lora"]["frequency_tx"] | 433775000;
  conf.lora.power           = data["lora"]["power"] | 20;
  conf.lora.spreadingFactor = data["lora"]["spreading_factor"] | 12;
  conf.lora.signalBandwidth = data["lora"]["signal_bandwidth"] | 125000;
  conf.lora.codingRate4     = data["lora"]["coding_rate4"] | 5;

  conf.ptt.active      = data["ptt_output"]["active"] | false;
  conf.ptt.io_pin      = data["ptt_output"]["io_pin"] | 4;
  conf.ptt.start_delay = data["ptt_output"]["start_delay"] | 0;
  conf.ptt.end_delay   = data["ptt_output"]["end_delay"] | 0;
  conf.ptt.reverse     = data["ptt_output"]["reverse"] | false;

  return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeConfiguration(Configuration conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
    logPrintlnE("Failed to open file for writing...");
    return;
  }
  DynamicJsonDocument data(2048);

  JsonArray beacons = data.createNestedArray("beacons");
  for (Configuration::Beacon beacon : conf.beacons) {
    JsonObject v  = beacons.createNestedObject();
    v["callsign"] = beacon.callsign;
    v["path"]     = beacon.path;
    v["message"]  = beacon.message;
    v["timeout"]  = beacon.timeout;
    v["symbol"]   = beacon.symbol;
    v["overlay"]  = beacon.overlay;

    v["smart_beacon"]["active"]      = beacon.smart_beacon.active;
    v["smart_beacon"]["turn_min"]    = beacon.smart_beacon.turn_min;
    v["smart_beacon"]["slow_rate"]   = beacon.smart_beacon.slow_rate;
    v["smart_beacon"]["slow_speed"]  = beacon.smart_beacon.slow_speed;
    v["smart_beacon"]["fast_rate"]   = beacon.smart_beacon.fast_rate;
    v["smart_beacon"]["fast_speed"]  = beacon.smart_beacon.fast_speed;
    v["smart_beacon"]["min_tx_dist"] = beacon.smart_beacon.min_tx_dist;
    v["smart_beacon"]["min_bcn"]     = beacon.smart_beacon.min_bcn;

    v["enhance_precision"] = beacon.enhance_precision;
  }

  data["debug"] = conf.debug;

  data["button"]["tx"]          = conf.button.tx;
  data["button"]["alt_message"] = conf.button.alt_message;

  data["lora"]["frequency_rx"]     = conf.lora.frequencyRx;
  data["lora"]["frequency_tx"]     = conf.lora.frequencyTx;
  data["lora"]["power"]            = conf.lora.power;
  data["lora"]["spreading_factor"] = conf.lora.spreadingFactor;
  data["lora"]["signal_bandwidth"] = conf.lora.signalBandwidth;
  data["lora"]["coding_rate4"]     = conf.lora.codingRate4;

  data["ptt_out"]["active"]      = conf.ptt.active;
  data["ptt_out"]["io_pin"]      = conf.ptt.io_pin;
  data["ptt_out"]["start_delay"] = conf.ptt.start_delay;
  data["ptt_out"]["end_delay"]   = conf.ptt.end_delay;
  data["ptt_out"]["reverse"]     = conf.ptt.reverse;

  serializeJson(data, file);
  file.close();
}
