#include <Arduino.h>
#include <LoRa.h>
#include <APRS-Decoder.h>
#include <axp20x.h>
#include <TinyGPS++.h>

#include "settings.h"
#include "display.h"

#define RXPin 12
#define TXPin 34

void setup_lora();
String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);

HardwareSerial ss(1);
AXP20X_Class axp;
TinyGPSPlus gps;
int next_update = -1;

void setup()
{
	Serial.begin(115200);
	setup_display();
	
	delay(500);
	Serial.println("[INFO] LoRa APRS iGate by OE5BPA (Peter Buchegger)");
	show_display("OE5BPA", "LoRa APRS iGate", "by Peter Buchegger", 2000);

	Wire.begin(SDA, SCL);
	if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS))
	{
		Serial.println("LoRa-APRS / Init / AXP192 Begin PASS");
	} else {
		Serial.println("LoRa-APRS / Init / AXP192 Begin FAIL");
	}
	axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
	axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
	axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
	axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
	axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
	axp.setDCDC1Voltage(3300);

	ss.begin(9600, SERIAL_8N1, TXPin, RXPin);

	setup_lora();
	
	delay(500);
}

#define BROADCAST_TIMEOUT 5

void loop()
{
	while (ss.available() > 0)
	{
		char c = ss.read();
		//Serial.print(c);
		gps.encode(c);
	}

	if(gps.time.isUpdated())
	{
		if(gps.time.isValid()
                        && (next_update == gps.time.minute() || next_update == -1)
			&& gps.location.isValid()
                        && gps.location.isUpdated())
		{
			APRSMessage msg;
			msg.setSource("OE5BPA-9");
			msg.setDestination("APRS");
			char body_char[50];
			sprintf(body_char, "=%s/%s>LoRa APRS Tracker test", create_lat_aprs(gps.location.rawLat()).c_str(), create_long_aprs(gps.location.rawLng()).c_str());
			msg.getAPRSBody()->setData(String(body_char));
			Serial.println(msg.encode());
			//LoRa.beginPacket();
			//LoRa.write((const uint8_t*)buffer.c_str(), buffer.length());
			//LoRa.endPacket();
			next_update = (gps.time.minute() + BROADCAST_TIMEOUT) % 60;
		}
		show_display("OE5BPA",
			String("Time: ") + gps.time.hour() + String(":") + gps.time.minute() + String(":") + gps.time.second(),
			String("Date: ") + gps.date.day()  + String(".") + gps.date.month()  + String(".") + gps.date.year(),
			String("Sat's: ") + gps.satellites.value() + String(" HDOP: ") + gps.hdop.hdop(),
			String("Lat: ") + gps.location.lat() + String(" Lng: ") + gps.location.lng(),
			String("") + create_lat_aprs(gps.location.rawLat()) + String(" ") + create_long_aprs(gps.location.rawLng())
			);
	}

	if(millis() > 5000 && gps.charsProcessed() < 10)
	{
		Serial.println("No GPS detected!");
	}
	
	/*String buffer = "OE5BPA-7>APRS:=4819.82NI01418.68E&LoRa Tracker test\n";
	LoRa.beginPacket();
	LoRa.write((const uint8_t*)buffer.c_str(), buffer.length());
	LoRa.endPacket();
	Serial.print("[INFO] Package sent: ");
	Serial.println(buffer);
	show_display("OE5BPA", "Package sent", buffer);
	delay(30000);*/
}

void setup_lora()
{
	Serial.println("[INFO] Set SPI pins!");
	SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
	LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
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
	Serial.println("[INFO] LoRa init done!");
	show_display("INFO", "LoRa init done!", 2000);
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
