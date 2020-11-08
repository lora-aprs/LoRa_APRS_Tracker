#include <Arduino.h>
#include <LoRa.h>
#include <APRS-Decoder.h>
#include <TinyGPS++.h>
#include <TimeLib.h>
#include <WiFi.h>

#include "settings.h"
#include "display.h"
#include "pins.h"
#include "power_management.h"

#include "power_management.h"
PowerManagement powerManagement;

HardwareSerial ss(1);
TinyGPSPlus gps;

void setup_lora();
void setup_gps();
String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);
String createNowStringTime();

static time_t nowTimeStamp = -1;
static time_t nextBeaconTimeStamp = -1;
static tmElements_t nextBeaconStruct;
static bool send_update = true;

// cppcheck-suppress unusedFunction
void setup()
{
	Serial.begin(115200);

#ifdef TTGO_T_Beam_V1_0
	Wire.begin(SDA, SCL);
	if (!powerManagement.begin(Wire))
	{
		Serial.println("[INFO] AXP192 init done!");
	}
	else
	{
		Serial.println("[ERROR] AXP192 init failed!");
	}
	powerManagement.activateLoRa();
	powerManagement.activateOLED();
	powerManagement.activateGPS();
#endif
	
	delay(500);
	Serial.println("[INFO] LoRa APRS Tracker by OE5BPA (Peter Buchegger)");
	setup_display();
	show_display("OE5BPA", "LoRa APRS Tracker", "by Peter Buchegger", 2000);

	setup_gps();
	setup_lora();

	// make sure wifi and bt is off as we don't need it:
	WiFi.mode(WIFI_OFF);
	btStop();

	delay(500);
	Serial.println("[INFO] setup done...");
}

String toDoubleInt(int number)
{
	if(number < 10)
	{
		return "0" + String(number);
	}
	return String(number);
}

// cppcheck-suppress unusedFunction
void loop()
{
	while (ss.available() > 0)
	{
		char c = ss.read();
		//Serial.print(c);
		gps.encode(c);
	}

	bool gps_time_update = gps.time.isUpdated();

	if(gps.time.isValid())
	{
		setTime(gps.time.hour(),gps.time.minute(),gps.time.second(),gps.date.day(),gps.date.month(),gps.date.year());
		nowTimeStamp = now();

		if (nextBeaconTimeStamp <= nowTimeStamp || nextBeaconTimeStamp == -1)
		{
			send_update = true;
		}
	}

	if(send_update && gps.location.isValid() && gps.location.isUpdated())
	{
		nextBeaconTimeStamp = nowTimeStamp + (BEACON_TIMEOUT * SECS_PER_MIN);
		breakTime(nextBeaconTimeStamp, nextBeaconStruct);
		send_update = false;

		APRSMessage msg;
		msg.setSource(CALL);
		msg.setDestination("APLT0");
		String lat = create_lat_aprs(gps.location.rawLat());
		String lng = create_long_aprs(gps.location.rawLng());
		msg.getAPRSBody()->setData(String("=") + lat + SYMBOL_OVERLAY + lng + SYMBOL_CODE + BEACON_MESSAGE);
		String data = msg.encode();
		Serial.println(data);
		show_display("<< TX >>", data);
		LoRa.beginPacket();
		// Header:
		LoRa.write('<');
		LoRa.write(0xFF);
		LoRa.write(0x01);
		// APRS Data:
		LoRa.write((const uint8_t *)data.c_str(), data.length());
		LoRa.endPacket();
	}

	if(gps_time_update)
	{
#ifdef TTGO_T_Beam_V1_0
		String batteryVoltage(powerManagement.getBatteryVoltage(), 2);
		String batteryChargeCurrent(powerManagement.getBatteryChargeDischargeCurrent(), 0);
#endif

		show_display(CALL,
			createNowStringTime(),
			String("Sats: ") + gps.satellites.value() + String(" HDOP: ") + gps.hdop.hdop(),
			String("Nxt Bcn: ") + toDoubleInt(nextBeaconStruct.Hour) + String(":") + toDoubleInt(nextBeaconStruct.Minute)
#ifdef TTGO_T_Beam_V1_0
			, String("Bat: ") + batteryVoltage + String("V ") + batteryChargeCurrent + String("mA")
#endif
			);
	}

	if(millis() > 5000 && gps.charsProcessed() < 10)
	{
		Serial.println("No GPS detected!");
	}
}

void setup_lora()
{
	Serial.println("[INFO] Set SPI pins!");
	SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
	LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
	Serial.println("[INFO] Set LoRa pins!");

	long freq = 433775000;
	Serial.print("[INFO] frequency: ");
	Serial.println(freq);
	if (!LoRa.begin(freq)) {
		Serial.println("[ERROR] Starting LoRa failed!");
		show_display("ERROR", "Starting LoRa failed!");
		while (1);
	}
	LoRa.setSpreadingFactor(12);
	LoRa.setSignalBandwidth(125E3);
	LoRa.setCodingRate4(5);
	LoRa.enableCrc();

	LoRa.setTxPower(20);
	Serial.println("[INFO] LoRa init done!");
	show_display("INFO", "LoRa init done!", 2000);
}

void setup_gps()
{
	ss.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
}

String create_lat_aprs(RawDegrees lat)
{
	char str[20];
	char n_s = 'N';
	if(lat.negative)
	{
		n_s = 'S';
	}
	sprintf(str, "%02d%05.2f%c", lat.deg, lat.billionths / 1000000000.0 * 60.0, n_s);
	String lat_str(str);
	return lat_str;
}

String create_long_aprs(RawDegrees lng)
{
	char str[20];
	char e_w = 'E';
	if(lng.negative)
	{
		e_w = 'W';
	}
	sprintf(str, "%03d%05.2f%c", lng.deg, lng.billionths / 1000000000.0 * 60.0, e_w);
	String lng_str(str);
	return lng_str;
}

String createNowStringTime()
{
	char line[30];
	sprintf(line, "%02d.%02d.%04d %02d:%02d:%02d", day(), month(), year(), hour(), minute(), second());
	return String(line);
}
