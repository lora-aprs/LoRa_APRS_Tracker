#include <APRS-Decoder.h>
#include <Arduino.h>
#include <LoRa.h>
#include <OneButton.h>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <logger.h>

#include "BeaconManager.h"
#include "configuration.h"
#include "display.h"
#include "pins.h"
#include "power_management.h"

#define VERSION "22.19.0"

logging::Logger logger;

Configuration Config;
BeaconManager BeaconMan;

PowerManagement powerManagement;
OneButton       userButton = OneButton(BUTTON_PIN, true, true);

HardwareSerial ss(1);
TinyGPSPlus    gps;

void setup_gps();
void load_config();
void setup_lora();

String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);
String create_lat_aprs_dao(RawDegrees lat);
String create_long_aprs_dao(RawDegrees lng);
String create_dao_aprs(RawDegrees lat, RawDegrees lng);
String createDateString(time_t t);
String createTimeString(time_t t);
String getSmartBeaconState();
String padding(unsigned int number, unsigned int width);

static bool send_update          = true;
static bool display_toggle_value = true;

static void handle_tx_click() {
  send_update = true;
}

static void handle_next_beacon() {
  BeaconMan.loadNextBeacon();
  show_display(BeaconMan.getCurrentBeaconConfig()->callsign, BeaconMan.getCurrentBeaconConfig()->message, 2000);
}

static void toggle_display() {
  display_toggle_value = !display_toggle_value;
  display_toggle(display_toggle_value);
  if (display_toggle_value) {
    setup_display();
  }
}

// cppcheck-suppress unusedFunction
void setup() {
  Serial.begin(115200);

#ifdef TTGO_T_Beam_V1_0
  Wire.begin(SDA, SCL);
  if (!powerManagement.begin(Wire)) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "AXP192", "init done!");
  } else {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "AXP192", "init failed!");
  }
  powerManagement.activateLoRa();
  powerManagement.activateOLED();
  powerManagement.activateGPS();
  powerManagement.activateMeasurement();
#endif

  delay(500);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "LoRa APRS Tracker by OE5BPA (Peter Buchegger)");
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Version: " VERSION);
  setup_display();

  show_display("OE5BPA", "LoRa APRS Tracker", "by Peter Buchegger", "Version: " VERSION, 2000);
  load_config();

  setup_gps();
  setup_lora();

  if (Config.ptt.active) {
    pinMode(Config.ptt.io_pin, OUTPUT);
    digitalWrite(Config.ptt.io_pin, Config.ptt.reverse ? HIGH : LOW);
  }

  // make sure wifi and bt is off as we don't need it:
  WiFi.mode(WIFI_OFF);
  btStop();

  if (Config.button.tx) {
    // attach TX action to user button (defined by BUTTON_PIN)
    userButton.attachClick(handle_tx_click);
  }

  if (Config.button.alt_message) {
    userButton.attachLongPressStart(handle_next_beacon);
  }
  userButton.attachDoubleClick(toggle_display);

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Smart Beacon is: %s", getSmartBeaconState());
  show_display("INFO", "Smart Beacon is " + getSmartBeaconState(), 1000);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "setup done...");
  delay(500);
}

// cppcheck-suppress unusedFunction
void loop() {
  userButton.tick();

  if (Config.debug) {
    while (Serial.available() > 0) {
      char c = Serial.read();
      // Serial.print(c);
      gps.encode(c);
    }
  } else {
    while (ss.available() > 0) {
      char c = ss.read();
      // Serial.print(c);
      gps.encode(c);
    }
  }

  bool          gps_time_update      = gps.time.isUpdated();
  bool          gps_loc_update       = gps.location.isUpdated();
  static bool   gps_loc_update_valid = false;
  static time_t nextBeaconTimeStamp  = -1;

  if (gps.location.isValid() != gps_loc_update_valid) {
    gps_loc_update_valid = gps.location.isValid();

    if (gps_loc_update_valid) {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Loop", "GPS fix state went to VALID");
    } else {
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Loop", "GPS fix state went to INVALID");
    }
  }

  static double       currentHeading          = 0;
  static double       previousHeading         = 0;
  static unsigned int rate_limit_message_text = 0;

  if (gps.time.isValid()) {
    setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());

    if (gps_loc_update && nextBeaconTimeStamp <= now()) {
      send_update = true;
      if (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active) {
        currentHeading = gps.course.deg();
        // enforce message text on slowest Config.smart_beacon.slow_rate
        rate_limit_message_text = 0;
      } else {
        // enforce message text every n's Config.beacon.timeout frame
        if (BeaconMan.getCurrentBeaconConfig()->timeout * rate_limit_message_text > 30) {
          rate_limit_message_text = 0;
        }
      }
    }
  }

  static double   lastTxLat       = 0.0;
  static double   lastTxLng       = 0.0;
  static double   lastTxdistance  = 0.0;
  static uint32_t txInterval      = 60000L; // Initial 60 secs internal
  static uint32_t lastTxTime      = millis();
  static int      speed_zero_sent = 0;

  static bool   BatteryIsConnected   = false;
  static String batteryVoltage       = "";
  static String batteryChargeCurrent = "";
#ifdef TTGO_T_Beam_V1_0
  static unsigned int rate_limit_check_battery = 0;
  if (!(rate_limit_check_battery++ % 60))
    BatteryIsConnected = powerManagement.isBatteryConnect();
  if (BatteryIsConnected) {
    batteryVoltage       = String(powerManagement.getBatteryVoltage(), 2);
    batteryChargeCurrent = String(powerManagement.getBatteryChargeDischargeCurrent(), 0);
  }
#endif

  if (powerManagement.isChargeing()) {
    powerManagement.enableChgLed();
  } else {
    powerManagement.disableChgLed();
  }

  if (!send_update && gps_loc_update && BeaconMan.getCurrentBeaconConfig()->smart_beacon.active) {
    uint32_t lastTx = millis() - lastTxTime;
    currentHeading  = gps.course.deg();
    lastTxdistance  = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), lastTxLat, lastTxLng);
    if (lastTx >= txInterval) {
      // Trigger Tx Tracker when Tx interval is reach
      // Will not Tx if stationary bcos speed < 5 and lastTxDistance < 20
      if (lastTxdistance > 20) {
        send_update = true;
      }
    }

    if (!send_update) {
      // Get headings and heading delta
      double headingDelta = abs(previousHeading - currentHeading);

      if (lastTx > BeaconMan.getCurrentBeaconConfig()->smart_beacon.min_bcn * 1000) {
        // Check for heading more than 25 degrees
        if (headingDelta > BeaconMan.getCurrentBeaconConfig()->smart_beacon.turn_min && lastTxdistance > BeaconMan.getCurrentBeaconConfig()->smart_beacon.min_tx_dist) {
          send_update = true;
        }
      }
    }
  }

  if (send_update && gps_loc_update) {
    send_update = false;

    nextBeaconTimeStamp = now() + (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active ? BeaconMan.getCurrentBeaconConfig()->smart_beacon.slow_rate : (BeaconMan.getCurrentBeaconConfig()->timeout * SECS_PER_MIN));

    APRSMessage msg;
    String      lat;
    String      lng;
    String      dao;

    msg.setSource(BeaconMan.getCurrentBeaconConfig()->callsign);
    msg.setPath(BeaconMan.getCurrentBeaconConfig()->path);
    msg.setDestination("APLT00");

    if (!BeaconMan.getCurrentBeaconConfig()->enhance_precision) {
      lat = create_lat_aprs(gps.location.rawLat());
      lng = create_long_aprs(gps.location.rawLng());
    } else {
      lat = create_lat_aprs_dao(gps.location.rawLat());
      lng = create_long_aprs_dao(gps.location.rawLng());
      dao = create_dao_aprs(gps.location.rawLat(), gps.location.rawLng());
    }

    String alt     = "";
    int    alt_int = max(-99999, min(999999, (int)gps.altitude.feet()));
    if (alt_int < 0) {
      alt = "/A=-" + padding(alt_int * -1, 5);
    } else {
      alt = "/A=" + padding(alt_int, 6);
    }

    String course_and_speed = "";
    int    speed_int        = max(0, min(999, (int)gps.speed.knots()));
    if (speed_zero_sent < 3) {
      String speed      = padding(speed_int, 3);
      int    course_int = max(0, min(360, (int)gps.course.deg()));
      /* course in between 1..360 due to aprs spec */
      if (course_int == 0) {
        course_int = 360;
      }
      String course    = padding(course_int, 3);
      course_and_speed = course + "/" + speed;
    }
    if (speed_int == 0) {
      /* speed is 0.
         we send 3 packets with speed zero (so our friends know we stand still).
         After that, we save airtime by not sending speed/course 000/000.
         Btw, even if speed we really do not move, measured course is changeing
         (-> no useful / even wrong info)
      */
      if (speed_zero_sent < 3) {
        speed_zero_sent += 1;
      }
    } else {
      speed_zero_sent = 0;
    }

    String aprsmsg;
    aprsmsg = "!" + lat + BeaconMan.getCurrentBeaconConfig()->overlay + lng + BeaconMan.getCurrentBeaconConfig()->symbol + course_and_speed + alt;
    // message_text every 10's packet (i.e. if we have beacon rate 1min at high
    // speed -> every 10min). May be enforced above (at expirey of smart beacon
    // rate (i.e. every 30min), or every third packet on static rate (i.e.
    // static rate 10 -> every third packet)
    if (!(rate_limit_message_text++ % 10)) {
      aprsmsg += BeaconMan.getCurrentBeaconConfig()->message;
    }
    if (BatteryIsConnected) {
      aprsmsg += " -  _Bat.: " + batteryVoltage + "V - Cur.: " + batteryChargeCurrent + "mA";
    }

    if (BeaconMan.getCurrentBeaconConfig()->enhance_precision) {
      aprsmsg += " " + dao;
    }

    msg.getBody()->setData(aprsmsg);
    String data = msg.encode();
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_DEBUG, "Loop", "%s", data);
    show_display("<< TX >>", data);

    if (Config.ptt.active) {
      digitalWrite(Config.ptt.io_pin, Config.ptt.reverse ? LOW : HIGH);
      delay(Config.ptt.start_delay);
    }

    LoRa.beginPacket();
    // Header:
    LoRa.write('<');
    LoRa.write(0xFF);
    LoRa.write(0x01);
    // APRS Data:
    LoRa.write((const uint8_t *)data.c_str(), data.length());
    LoRa.endPacket();

    if (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active) {
      lastTxLat       = gps.location.lat();
      lastTxLng       = gps.location.lng();
      previousHeading = currentHeading;
      lastTxdistance  = 0.0;
      lastTxTime      = millis();
    }

    if (Config.ptt.active) {
      delay(Config.ptt.end_delay);
      digitalWrite(Config.ptt.io_pin, Config.ptt.reverse ? HIGH : LOW);
    }
  }

  if (gps_time_update) {

    show_display(BeaconMan.getCurrentBeaconConfig()->callsign, createDateString(now()) + "   " + createTimeString(now()), String("Sats: ") + gps.satellites.value() + " HDOP: " + gps.hdop.hdop(), String("Next Bcn: ") + (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active ? "~" : "") + createTimeString(nextBeaconTimeStamp), BatteryIsConnected ? (String("Bat: ") + batteryVoltage + "V, " + batteryChargeCurrent + "mA") : "Powered via USB", String("Smart Beacon: " + getSmartBeaconState()));

    if (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active) {
      // Change the Tx internal based on the current speed
      int curr_speed = (int)gps.speed.kmph();
      if (curr_speed < BeaconMan.getCurrentBeaconConfig()->smart_beacon.slow_speed) {
        txInterval = BeaconMan.getCurrentBeaconConfig()->smart_beacon.slow_rate * 1000;
      } else if (curr_speed > BeaconMan.getCurrentBeaconConfig()->smart_beacon.fast_speed) {
        txInterval = BeaconMan.getCurrentBeaconConfig()->smart_beacon.fast_rate * 1000;
      } else {
        /* Interval inbetween low and high speed
           min(slow_rate, ..) because: if slow rate is 300s at slow speed <=
           10km/h and fast rate is 60s at fast speed >= 100km/h everything below
           current speed 20km/h (100*60/20 = 300) is below slow_rate.
           -> In the first check, if curr speed is 5km/h (which is < 10km/h), tx
           interval is 300s, but if speed is 6km/h, we are landing in this
           section, what leads to interval 100*60/6 = 1000s (16.6min) -> this
           would lead to decrease of beacon rate in between 5 to 20 km/h. what
           is even below the slow speed rate.
        */
        txInterval = min(BeaconMan.getCurrentBeaconConfig()->smart_beacon.slow_rate, BeaconMan.getCurrentBeaconConfig()->smart_beacon.fast_speed * BeaconMan.getCurrentBeaconConfig()->smart_beacon.fast_rate / curr_speed) * 1000;
      }
    }
  }

  if ((Config.debug == false) && (millis() > 5000 && gps.charsProcessed() < 10)) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "GPS",
               "No GPS frames detected! Try to reset the GPS Chip with this "
               "firmware: https://github.com/lora-aprs/TTGO-T-Beam_GPS-reset");
    show_display("No GPS frames detected!", "Try to reset the GPS Chip", "https://github.com/lora-aprs/TTGO-T-Beam_GPS-reset", 2000);
  }
}

void load_config() {
  ConfigurationManagement confmg("/tracker.json");
  Config = confmg.readConfiguration();
  BeaconMan.loadConfig(Config.beacons);
  if (BeaconMan.getCurrentBeaconConfig()->callsign == "NOCALL-10") {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "Config",
               "You have to change your settings in 'data/tracker.json' and "
               "upload it via \"Upload File System image\"!");
    show_display("ERROR", "You have to change your settings in 'data/tracker.json' and "
                          "upload it via \"Upload File System image\"!");
    while (true) {
    }
  }
}

void setup_lora() {
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Set SPI pins!");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Set LoRa pins!");
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  long freq = Config.lora.frequencyTx;
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "frequency: %d", freq);
  if (!LoRa.begin(freq)) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Starting LoRa failed!");
    show_display("ERROR", "Starting LoRa failed!");
    while (true) {
    }
  }
  LoRa.setSpreadingFactor(Config.lora.spreadingFactor);
  LoRa.setSignalBandwidth(Config.lora.signalBandwidth);
  LoRa.setCodingRate4(Config.lora.codingRate4);
  LoRa.enableCrc();

  LoRa.setTxPower(Config.lora.power);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa init done!");
  show_display("INFO", "LoRa init done!", 2000);
}

void setup_gps() {
  ss.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
}

char *s_min_nn(uint32_t min_nnnnn, int high_precision) {
  /* min_nnnnn: RawDegrees billionths is uint32_t by definition and is n'telth
   * degree (-> *= 6 -> nn.mmmmmm minutes) high_precision: 0: round at decimal
   * position 2. 1: round at decimal position 4. 2: return decimal position 3-4
   * as base91 encoded char
   */

  static char buf[6];
  min_nnnnn = min_nnnnn * 0.006;

  if (high_precision) {
    if ((min_nnnnn % 10) >= 5 && min_nnnnn < 6000000 - 5) {
      // round up. Avoid overflow (59.999999 should never become 60.0 or more)
      min_nnnnn = min_nnnnn + 5;
    }
  } else {
    if ((min_nnnnn % 1000) >= 500 && min_nnnnn < (6000000 - 500)) {
      // round up. Avoid overflow (59.9999 should never become 60.0 or more)
      min_nnnnn = min_nnnnn + 500;
    }
  }

  if (high_precision < 2)
    sprintf(buf, "%02u.%02u", (unsigned int)((min_nnnnn / 100000) % 100), (unsigned int)((min_nnnnn / 1000) % 100));
  else
    sprintf(buf, "%c", (char)((min_nnnnn % 1000) / 11) + 33);
  // Like to verify? type in python for i.e. RawDegrees billions 566688333: i =
  // 566688333; "%c" % (int(((i*.0006+0.5) % 100)/1.1) +33)
  return buf;
}

String create_lat_aprs(RawDegrees lat) {
  char str[20];
  char n_s = 'N';
  if (lat.negative) {
    n_s = 'S';
  }
  // we like sprintf's float up-rounding.
  // but sprintf % may round to 60.00 -> 5360.00 (53° 60min is a wrong notation
  // ;)
  sprintf(str, "%02d%s%c", lat.deg, s_min_nn(lat.billionths, 0), n_s);
  String lat_str(str);
  return lat_str;
}

String create_lat_aprs_dao(RawDegrees lat) {
  // round to 4 digits and cut the last 2
  char str[20];
  char n_s = 'N';
  if (lat.negative) {
    n_s = 'S';
  }
  // we need sprintf's float up-rounding. Must be the same principle as in
  // aprs_dao(). We cut off the string to two decimals afterwards. but sprintf %
  // may round to 60.0000 -> 5360.0000 (53° 60min is a wrong notation ;)
  sprintf(str, "%02d%s%c", lat.deg, s_min_nn(lat.billionths, 1 /* high precision */), n_s);
  String lat_str(str);
  return lat_str;
}

String create_long_aprs(RawDegrees lng) {
  char str[20];
  char e_w = 'E';
  if (lng.negative) {
    e_w = 'W';
  }
  sprintf(str, "%03d%s%c", lng.deg, s_min_nn(lng.billionths, 0), e_w);
  String lng_str(str);
  return lng_str;
}

String create_long_aprs_dao(RawDegrees lng) {
  // round to 4 digits and cut the last 2
  char str[20];
  char e_w = 'E';
  if (lng.negative) {
    e_w = 'W';
  }
  sprintf(str, "%03d%s%c", lng.deg, s_min_nn(lng.billionths, 1 /* high precision */), e_w);
  String lng_str(str);
  return lng_str;
}

String create_dao_aprs(RawDegrees lat, RawDegrees lng) {
  // !DAO! extension, use Base91 format for best precision
  // /1.1 : scale from 0-99 to 0-90 for base91, int(... + 0.5): round to nearest
  // integer https://metacpan.org/dist/Ham-APRS-FAP/source/FAP.pm
  // http://www.aprs.org/aprs12/datum.txt
  //

  char str[10];
  sprintf(str, "!w%s", s_min_nn(lat.billionths, 2));
  sprintf(str + 3, "%s!", s_min_nn(lng.billionths, 2));
  String dao_str(str);
  return dao_str;
}

String createDateString(time_t t) {
  return String(padding(day(t), 2) + "." + padding(month(t), 2) + "." + padding(year(t), 4));
}

String createTimeString(time_t t) {
  return String(padding(hour(t), 2) + ":" + padding(minute(t), 2) + ":" + padding(second(t), 2));
}

String getSmartBeaconState() {
  if (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active) {
    return "On";
  }
  return "Off";
}

String padding(unsigned int number, unsigned int width) {
  String result;
  String num(number);
  if (num.length() > width) {
    width = num.length();
  }
  for (unsigned int i = 0; i < width - num.length(); i++) {
    result.concat('0');
  }
  result.concat(num);
  return result;
}
