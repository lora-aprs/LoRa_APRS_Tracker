#include <Arduino.h>
#include <LoRa.h>
#include <APRS-Decoder.h>

#include "settings.h"
#include "display.h"


void setup_lora();

void setup()
{
	Serial.begin(115200);
	setup_display();
	
	delay(500);
	Serial.println("[INFO] LoRa APRS iGate by OE5BPA (Peter Buchegger)");
	show_display("OE5BPA", "LoRa APRS iGate", "by Peter Buchegger", 2000);

	setup_lora();
	
	delay(500);
}

void loop()
{
	String buffer = "OE5BPA-7>APRS:=4819.82NI01418.68E&LoRa Tracker test\n";
	LoRa.beginPacket();
	LoRa.write((const uint8_t*)buffer.c_str(), buffer.length());
	LoRa.endPacket();
	Serial.print("[INFO] Package sent: ");
	Serial.println(buffer);
	show_display("OE5BPA", "Package sent", buffer);
	delay(30000);
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
