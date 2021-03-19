#include <Arduino.h>
#include <LoRa.h>
#include <APRS-Decoder.h>
#include <TinyGPS++.h>
#include <TimeLib.h>
#include <WiFi.h>
#include "display.h"
#include "pins.h"
#include "power_management.h"
#include "configuration.h"
Configuration Config;

#include "power_management.h"
PowerManagement powerManagement;

#include "logger.h"

HardwareSerial ss(1);
TinyGPSPlus gps;

void load_config();
void setup_lora();
void setup_gps();

String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);
String createDateString(time_t t);
String createTimeString(time_t t);
String getSmartBeaconState();
String padding(unsigned int number, unsigned int width);

// cppcheck-suppress unusedFunction
void setup()
{
	Serial.begin(115200);

#ifdef TTGO_T_Beam_V1_0
	Wire.begin(SDA, SCL);
	if (!powerManagement.begin(Wire))
	{
		logPrintlnI("AXP192 init done!");
	}
	else
	{
		logPrintlnE("AXP192 init failed!");
	}
	powerManagement.activateLoRa();
	powerManagement.activateOLED();
	powerManagement.activateGPS();
	powerManagement.activateMeasurement();
#endif
	
	delay(500);
	logPrintlnI("LoRa APRS Tracker by OE5BPA (Peter Buchegger)");
	setup_display();
	
	show_display("OE5BPA", "LoRa APRS Tracker", "by Peter Buchegger", 2000);
	load_config();
	
	setup_gps();
	setup_lora();

	// make sure wifi and bt is off as we don't need it:
	WiFi.mode(WIFI_OFF);
	btStop();

	logPrintlnI("Smart Beacon is " + getSmartBeaconState());
	show_display("INFO", "Smart Beacon is " + getSmartBeaconState(), 1000);
	logPrintlnI("setup done...");
	delay(500);
}

// cppcheck-suppress unusedFunction
void loop()
{
	if(Config.debug)
	{
		while(Serial.available() > 0)
		{
			char c = Serial.read();
			//Serial.print(c);
			gps.encode(c);
		}
	}
	else
	{
		while(ss.available() > 0)
		{
			char c = ss.read();
			//Serial.print(c);
			gps.encode(c);
		}
	}

	bool gps_time_update = gps.time.isUpdated();
	bool gps_loc_update = gps.location.isUpdated();
	static time_t nextBeaconTimeStamp = -1;
	static bool send_update = true;

	if(gps.time.isValid())
	{
		setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());

		if(nextBeaconTimeStamp <= now())
		{
			send_update = true;
		}
	}

	static double lastTxLat = 0.0;
	static double lastTxLng = 0.0;
	static double lastTxdistance = 0.0;
	static unsigned long txInterval = 60000L;  // Initial 60 secs internal

	static double currentHeading = 0;
	static double previousHeading = 0;

	static int lastTxTime = millis();

	if(Config.smart_beacon.active)
	{
		if(gps_loc_update)
		{
			lastTxdistance = TinyGPSPlus::distanceBetween(gps.location.lat(), gps.location.lng(), lastTxLat, lastTxLng);
				
			// Get headings and heading delta
			currentHeading = gps.course.deg();
			double headingDelta = abs(previousHeading - currentHeading);

			int lastTx = millis() - lastTxTime;
			
			if(lastTx > Config.smart_beacon.min_bcn * 1000)
			{
				// Check for heading more than 25 degrees
				if(headingDelta > Config.smart_beacon.turn_min && lastTxdistance > Config.smart_beacon.min_tx_dist)
				{
					send_update = true;
				}
			}

			if(lastTx >= txInterval)
			{
				// Trigger Tx Tracker when Tx interval is reach 
				// Will not Tx if stationary bcos speed < 5 and lastTxDistance < 20
				if (lastTxdistance > 20)
				{
					send_update = true;
				}
			}
		}
	}

	if(send_update && gps_loc_update)
	{
		send_update = false;
		nextBeaconTimeStamp = now() + (Config.beacon.timeout * SECS_PER_MIN);

		APRSMessage msg;
		msg.setSource(Config.callsign);
		msg.setDestination("APLT00");
		String lat = create_lat_aprs(gps.location.rawLat());
		String lng = create_long_aprs(gps.location.rawLng());
		String alt = padding((int)gps.altitude.feet(), 6);
		String course = padding((int)gps.course.deg(), 3);
		String speed = padding((int)gps.speed.knots(), 3);
		
#ifdef 	TTGO_T_Beam_V1_0
		String batteryVoltage(powerManagement.getBatteryVoltage(), 2);
		String batteryChargeCurrent(powerManagement.getBatteryChargeDischargeCurrent(), 0);
		msg.getAPRSBody()->setData(String("=") + lat + Config.beacon.overlay + lng + Config.beacon.symbol + course + "/" + speed + "/A=" + alt + Config.beacon.message + " - Bat.: " + batteryVoltage + "V - Cur.: " + batteryChargeCurrent + "mA");
#else
		msg.getAPRSBody()->setData(String("=") + lat + Config.beacon.overlay + lng + Config.beacon.symbol + course + "/" + speed + "/A=" + alt + Config.beacon.message);
#endif
		String data = msg.encode();
		logPrintlnD(data);
		show_display("<< TX >>", data);
		LoRa.beginPacket();
		// Header:
		LoRa.write('<');
		LoRa.write(0xFF);
		LoRa.write(0x01);
		// APRS Data:
		LoRa.write((const uint8_t *)data.c_str(), data.length());
		LoRa.endPacket();

		if (Config.smart_beacon.active)
		{
			lastTxLat = gps.location.lat();
			lastTxLng = gps.location.lng();
			previousHeading = currentHeading;
			lastTxdistance = 0.0;
			lastTxTime = millis();
		}
	}

	if(gps_time_update)
	{
#ifdef TTGO_T_Beam_V1_0
		String batteryVoltage(powerManagement.getBatteryVoltage(), 2);
		String batteryChargeCurrent(powerManagement.getBatteryChargeDischargeCurrent(), 0);
#endif

		show_display(Config.callsign,
			createDateString(now()) + " " + createTimeString(now()),
			String("Sats: ") + gps.satellites.value() + " HDOP: " + gps.hdop.hdop(),
			String("Nxt Bcn: ") + createTimeString(nextBeaconTimeStamp)
#ifdef TTGO_T_Beam_V1_0
			, String("Bat: ") + batteryVoltage + "V, " + batteryChargeCurrent + "mA"
#endif
			, String("Smart Beacon: " + getSmartBeaconState())
			);

		if(Config.smart_beacon.active)
		{
			// Change the Tx internal based on the current speed
			if(gps.speed.kmph() < 5)
			{
				txInterval = 300000; // Change Tx internal to 5 mins
			}
			else if(gps.speed.kmph() < Config.smart_beacon.slow_speed)
			{
				txInterval = Config.smart_beacon.slow_rate * 1000;
			}
			else if(gps.speed.kmph() > Config.smart_beacon.fast_speed)
			{
				txInterval = Config.smart_beacon.fast_rate * 1000;
			}
			else
			{
				// Interval inbetween low and high speed 
				txInterval = (Config.smart_beacon.fast_speed / gps.speed.kmph()) * Config.smart_beacon.fast_rate * 1000;
			}
		}
	}

	if ((Config.debug == false) && (millis() > 5000 && gps.charsProcessed() < 10))
	{
		logPrintlnE("No GPS frames detected! Try to reset the GPS Chip with this firmware: https://github.com/lora-aprs/TTGO-T-Beam_GPS-reset");
	}
}

void load_config()
{
	ConfigurationManagement confmg("/tracker.json");
	Config = confmg.readConfiguration();
	if(Config.callsign == "NOCALL-10")
	{
		logPrintlnE("You have to change your settings in 'data/is-cfg.json' and upload it via \"Upload File System image\"!");
		show_display("ERROR", "You have to change your settings in 'data/is-cfg.json' and upload it via \"Upload File System image\"!");
		while(true)
		{}
	}
}

void setup_lora()
{
	logPrintlnI("Set SPI pins!");
	SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
	logPrintlnI("Set LoRa pins!");
	LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

	long freq = Config.lora.frequencyTx;
	logPrintI("frequency: ");
	logPrintlnI(String(freq));
	if (!LoRa.begin(freq)) {
		logPrintlnE("Starting LoRa failed!");
		show_display("ERROR", "Starting LoRa failed!");
		while(true)
		{}
	}
	LoRa.setSpreadingFactor(Config.lora.spreadingFactor);
	LoRa.setSignalBandwidth(Config.lora.signalBandwidth);
	LoRa.setCodingRate4(Config.lora.codingRate4);
	LoRa.enableCrc();

	LoRa.setTxPower(Config.lora.power);
	logPrintlnI("LoRa init done!");
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

String createDateString(time_t t)
{
	return String(padding(day(t), 2) + "." + padding(month(t), 2) + "." + padding(year(t), 4));
}

String createTimeString(time_t t)
{
	if(t == -1)
	{
		return String("00:00:00");
	}
	return String(padding(hour(t), 2) + "." + padding(minute(t), 2) + "." + padding(second(t), 2));
}

String getSmartBeaconState()
{
	if (Config.smart_beacon.active)
	{
		return "On";
	}
	return "Off";
}

String padding(unsigned int number, unsigned int width)
{
	String result;
	String num(number);
	if(num.length() > width)
	{
		width = num.length();
	}
	for(unsigned int i = 0; i < width - num.length(); i++)
	{
		result.concat('0');
	}
	result.concat(num);
	return result;
}
