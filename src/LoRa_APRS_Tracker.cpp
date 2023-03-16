#include <APRS-Decoder.h>
#include <Arduino.h>
#include <LoRa.h>
#include <OneButton.h>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include <WiFi.h>

#include "configuration.h"
#include "display.h"
#include "pins.h"
#include "power_management.h"

#define VERSION "1.0"

// State of State machine
#define HasSynchGPS 1
#define PrepBeacon  2
#define Sleep       3

Configuration   mConfig;
PowerManagement powerManagement;
OneButton       userButton = OneButton(BUTTON_PIN, true, true);
HardwareSerial  ss(1);
TinyGPSPlus     gps;

void setup_gps();
void load_config();
void setup_lora();

String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);
String createDateString(time_t t);
String createTimeString(time_t t);
String padding(unsigned int number, unsigned int width);

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  powerManagement.begin(Wire);
  powerManagement.activateLoRa();
  powerManagement.activateOLED();
  powerManagement.activateGPS();
  powerManagement.activateMeasurement();
  // make sure wifi and bt is off as we don't need it:
  WiFi.mode(WIFI_OFF);
  btStop();

  setup_display();
  show_display("OE5BPA", "LoRa APRS Tracker", "by Peter Buchegger", "Version: " VERSION, 2000);
  load_config();

  setup_gps();
  setup_lora();
  // userButton.attachClick(handle_tx_click); // attach TX action to user button (defined by BUTTON_PIN)
  //  userButton.attachLongPressStart(handle_next_beacon);
  //  userButton.attachDoubleClick(toggle_display);
  esp_sleep_enable_timer_wakeup(mConfig.beacon.smart_beacon.slow_rate * 1000000);
  Serial.println("HI");
  String sM = String("Sleep for : ") + String(mConfig.beacon.smart_beacon.slow_rate, DEC) + String("s");
  Serial.println(sM);
  Serial.println("GO...");
  delay(1000);
}

void loop() {
  static unsigned int rate_limit_message_text = 0;
  static int          speed_zero_sent         = 0;
  static String       batteryVoltage          = "";
  static String       batteryChargeCurrent    = "";
  APRSMessage         msg;
  String              lat;
  String              lng;
  String              aprsmsg;
  static int          iState;

  userButton.tick();

  switch (iState) {
  case HasSynchGPS: {
    while (ss.available() > 0) {
      char c = ss.read();
      gps.encode(c);
    }

    if (gps.time.isValid()) {
      setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
      // Serial.println("GPS time ok");
    }

    if (gps.location.isValid()) {
      Serial.println("GPS pos ok");
      iState = PrepBeacon;
    }
    if (powerManagement.isCharging()) {
      powerManagement.enableChgLed();
    } else {
      powerManagement.disableChgLed();
    }
    delay(20);
    break;
  }

  case PrepBeacon: {
    if (powerManagement.isBatteryConnect()) {
      batteryVoltage       = String(powerManagement.getBatteryVoltage(), 2);
      batteryChargeCurrent = String(powerManagement.getBatteryChargeDischargeCurrent(), 0);
    }
    msg.setSource(mConfig.beacon.callsign);
    msg.setPath(mConfig.beacon.path);
    msg.setDestination("APLT00");
    lat = create_lat_aprs(gps.location.rawLat());
    lng = create_long_aprs(gps.location.rawLng());

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

    aprsmsg = "!" + lat + mConfig.beacon.overlay + lng + mConfig.beacon.symbol + course_and_speed + alt;
    // message_text every 10's packet (i.e. if we have beacon rate 1min at high
    // speed -> every 10min). May be enforced above (at expirey of smart beacon
    // rate (i.e. every 30min), or every third packet on static rate (i.e.
    // static rate 10 -> every third packet)
    if (!(rate_limit_message_text++ % 10)) {
      aprsmsg += mConfig.beacon.message;
    }

    if (!powerManagement.isCharging()) {
      aprsmsg += "VBat= " + batteryVoltage + "V Cur= " + batteryChargeCurrent + "mA";
    }

    msg.getBody()->setData(aprsmsg);
    String data = msg.encode();
    // show_display("<< TX >>", data);
    Serial.println("TX ok");
    // delay(1000);
    if (mConfig.ptt.active) {
      digitalWrite(mConfig.ptt.io_pin, mConfig.ptt.reverse ? LOW : HIGH);
      delay(mConfig.ptt.start_delay);
    }
    LoRa.beginPacket();
    LoRa.write('<'); // Header
    LoRa.write(0xFF);
    LoRa.write(0x01);
    LoRa.write((const uint8_t *)data.c_str(), data.length());
    LoRa.endPacket();

    show_display(mConfig.beacon.callsign, createDateString(now()) + "   " + createTimeString(now()), String("Sats: ") + gps.satellites.value() + " HDOP: " + gps.hdop.hdop(), !powerManagement.isCharging() ? (String("Bat: ") + batteryVoltage + "V, " + batteryChargeCurrent + "mA") : "Powered via USB");

    if (mConfig.ptt.active) {
      delay(mConfig.ptt.end_delay);
      digitalWrite(mConfig.ptt.io_pin, mConfig.ptt.reverse ? HIGH : LOW);
    }

    iState = Sleep;
    break;
  }
  case Sleep: {
    powerManagement.deactivateGPS();
    Serial.flush();
    esp_light_sleep_start();
    Serial.println("wake up");
    powerManagement.activateGPS();
    iState = HasSynchGPS;
    break;
  }

  default:
    iState = HasSynchGPS;
    break;
  }
}

void load_config() {
  ConfigurationManagement confmg("/tracker.json");
  mConfig = confmg.readConfiguration();
  if (mConfig.beacon.callsign == "NOCALL-7") {
    show_display("ERROR", "You have to change your settings in 'data/tracker.json' and "
                          "upload it via \"Upload File System image\"!");
    while (true) {
    }
  }
}

void setup_lora() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  long freq = mConfig.lora.frequencyTx;
  if (!LoRa.begin(freq)) {
    while (true) {
    }
  }
  LoRa.setSpreadingFactor(mConfig.lora.spreadingFactor);
  LoRa.setSignalBandwidth(mConfig.lora.signalBandwidth);
  LoRa.setCodingRate4(mConfig.lora.codingRate4);
  LoRa.enableCrc();
  LoRa.setTxPower(mConfig.lora.power);
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

String createDateString(time_t t) {
  return String(padding(day(t), 2) + "." + padding(month(t), 2) + "." + padding(year(t), 4));
}

String createTimeString(time_t t) {
  return String(padding(hour(t), 2) + ":" + padding(minute(t), 2) + ":" + padding(second(t), 2));
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
