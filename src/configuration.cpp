#include <SPIFFS.h>

#ifndef CPPCHECK
#include <ArduinoJson.h>
#endif

#include "configuration.h"

ConfigurationManagement::ConfigurationManagement(String FilePath) : mFilePath(FilePath) {
  if (!SPIFFS.begin(true)) {
    SPIFFS.format();
    SPIFFS.begin();
  }
}

// cppcheck-suppress unusedFunction
Configuration ConfigurationManagement::readConfiguration() {
  File file = SPIFFS.open(mFilePath);
  if (!file) {
    return Configuration();
  }
  DynamicJsonDocument  data(2048);
  DeserializationError error = deserializeJson(data, file);

  if (error) {
  }
  file.close();

  Configuration conf;

  conf.debug = data["debug"] | false;

  // conf.beacon.callsign ="NOCALL-7";
  conf.beacon.callsign = data["beacon"]["callsign"].as<String>();
  // conf.beacon.path ="WIDE1-1";
  conf.beacon.path = data["beacon"]["path"].as<String>();
  // conf.beacon.message="73";
  conf.beacon.message = data["beacon"]["message"].as<String>();
  conf.beacon.timeout = data["beacon"]["timeout"] | 1;
  // conf.beacon.symbol = "[";
  conf.beacon.symbol = data["beacon"]["symbol"].as<String>();
  // conf.beacon.overlay ="/";
  conf.beacon.overlay                  = data["beacon"]["overlay"].as<String>();
  conf.beacon.smart_beacon.active      = data["beacon"]["smart_beacon"]["active"] | false;
  conf.beacon.smart_beacon.turn_min    = data["beacon"]["smart_beacon"]["turn_min"] | 25;
  conf.beacon.smart_beacon.slow_rate   = data["beacon"]["smart_beacon"]["slow_rate"] | 120;
  conf.beacon.smart_beacon.slow_speed  = data["beacon"]["smart_beacon"]["slow_speed"] | 10;
  conf.beacon.smart_beacon.fast_rate   = data["beacon"]["smart_beacon"]["fast_rate"] | 10;
  conf.beacon.smart_beacon.fast_speed  = data["beacon"]["smart_beacon"]["fast_speed"] | 100;
  conf.beacon.smart_beacon.min_tx_dist = data["beacon"]["smart_beacon"]["min_tx_dist"] | 100;
  conf.beacon.smart_beacon.min_bcn     = data["beacon"]["smart_beacon"]["min_bcn"] | 5;
  conf.beacon.enhance_precision        = data["beacon"]["enhance_precision"] | false;

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

/* cppcheck-suppress unusedFunction
void ConfigurationManagement::writeConfiguration(Configuration conf) {
  File file = SPIFFS.open(mFilePath, "w");
  if (!file) {
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
}*/
