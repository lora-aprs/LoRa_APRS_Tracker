#include <SPIFFS.h>

#ifndef CPPCHECK
#include <ArduinoJson.h>
#endif

#include "configuration.h"
#include "logger.h"

ConfigurationManagement::ConfigurationManagement(String FilePath)
	: mFilePath(FilePath)
{
	if(!SPIFFS.begin(true))
	{
		logPrintlnE("Mounting SPIFFS was not possible. Trying to format SPIFFS...");
		SPIFFS.format();
		if(!SPIFFS.begin())
		{
			logPrintlnE("Formating SPIFFS was not okay!");
		}
	}
}

// cppcheck-suppress unusedFunction
Configuration ConfigurationManagement::readConfiguration()
{
	File file = SPIFFS.open(mFilePath);
	if(!file)
	{
		logPrintlnE("Failed to open file for reading...");
		return Configuration();
	}
	DynamicJsonDocument data(2048);
	DeserializationError error = deserializeJson(data, file);

	if(error)
	{
		logPrintlnE("Failed to read file, using default configuration.");
	}
	file.close();

	Configuration conf;
	if(data.containsKey("callsign"))
		conf.callsign				= data["callsign"].as<String>();
	conf.debug						= data["debug"]								| false;
	conf.enhance_precision			= data["enhance_precision"]					| false;
	if(data.containsKey("beacon") && data["beacon"].containsKey("message"))
		conf.beacon.message			= data["beacon"]["message"].as<String>();
	conf.beacon.timeout				= data["beacon"]["timeout"]					| 1;
	if(data.containsKey("beacon") && data["beacon"].containsKey("symbol"))
		conf.beacon.symbol			= data["beacon"]["symbol"].as<String>();
	if(data.containsKey("beacon") && data["beacon"].containsKey("overlay"))
		conf.beacon.overlay			= data["beacon"]["overlay"].as<String>() ;
	if(data.containsKey("beacon") && data["beacon"].containsKey("button_tx"))
		conf.beacon.button_tx			= data["beacon"]["button_tx"]			| false;

	conf.smart_beacon.active		= data["smart_beacon"]["active"]			| false;
	conf.smart_beacon.turn_min		= data["smart_beacon"]["turn_min"]			| 25;
	conf.smart_beacon.slow_rate		= data["smart_beacon"]["slow_rate"]			| 300;
	conf.smart_beacon.slow_speed	= data["smart_beacon"]["slow_speed"]		| 10;
	conf.smart_beacon.fast_rate		= data["smart_beacon"]["fast_rate"]			| 60;
	conf.smart_beacon.fast_speed	= data["smart_beacon"]["fast_speed"]		| 100;
	conf.smart_beacon.min_tx_dist	= data["smart_beacon"]["min_tx_dist"]		| 100;
	conf.smart_beacon.min_bcn		= data["smart_beacon"]["min_bcn"]			| 5;
	
	conf.lora.frequencyRx			= data["lora"]["frequency_rx"]				| 433775000;
	conf.lora.frequencyTx			= data["lora"]["frequency_tx"]				| 433775000;
	conf.lora.power					= data["lora"]["power"]						| 20;
	conf.lora.spreadingFactor		= data["lora"]["spreading_factor"]			| 12;
	conf.lora.signalBandwidth		= data["lora"]["signal_bandwidth"]			| 125000;
	conf.lora.codingRate4			= data["lora"]["coding_rate4"]				| 5;

	conf.ptt.active					= data["ptt_output"]["active"]				| false;
	conf.ptt.io_pin					= data["ptt_output"]["io_pin"]				| 4;
	conf.ptt.start_delay			= data["ptt_output"]["start_delay"]			| 0;
	conf.ptt.end_delay				= data["ptt_output"]["end_delay"]			| 0;
	conf.ptt.reverse				= data["ptt_output"]["reverse"]				| false;

	return conf;
}

// cppcheck-suppress unusedFunction
void ConfigurationManagement::writeConfiguration(Configuration conf)
{
	File file = SPIFFS.open(mFilePath, "w");
	if(!file)
	{
		logPrintlnE("Failed to open file for writing...");
		return;
	}
	DynamicJsonDocument data(2048);

	data["callsign"]						= conf.callsign;
	data["debug"]							= conf.debug;
	data["enhance_precision"]				= conf.enhance_precision;
	data["beacon"]["message"]				= conf.beacon.message;
	data["beacon"]["timeout"]				= conf.beacon.timeout;
	data["beacon"]["symbol"]				= conf.beacon.symbol;
	data["beacon"]["overlay"]				= conf.beacon.overlay;
	data["beacon"]["button_tx"]				= conf.beacon.button_tx;
	data["smart_beacon"]["active"]			= conf.smart_beacon.active;  
	data["smart_beacon"]["turn_min"] 		= conf.smart_beacon.turn_min;	
	data["smart_beacon"]["slow_rate"]		= conf.smart_beacon.slow_rate;	
	data["smart_beacon"]["slow_speed"]		= conf.smart_beacon.slow_speed;
	data["smart_beacon"]["fast_rate"]		= conf.smart_beacon.fast_rate;	  
	data["smart_beacon"]["fast_speed"]		= conf.smart_beacon.fast_speed;
	data["smart_beacon"]["min_tx_dist"] 	= conf.smart_beacon.min_tx_dist;
	data["smart_beacon"]["min_bcn"] 		= conf.smart_beacon.min_bcn ;

	data["lora"]["frequency_rx"]			= conf.lora.frequencyRx;
	data["lora"]["frequency_tx"]			= conf.lora.frequencyTx;
	data["lora"]["power"]					= conf.lora.power;
	data["lora"]["spreading_factor"]		= conf.lora.spreadingFactor;
	data["lora"]["signal_bandwidth"]		= conf.lora.signalBandwidth;
	data["lora"]["coding_rate4"]			= conf.lora.codingRate4;
	
	data["ptt_out"]["active"]				= conf.ptt.active;
	data["ptt_out"]["io_pin"] 				= conf.ptt.io_pin;
	data["ptt_out"]["start_delay"]			= conf.ptt.start_delay;
	data["ptt_out"]["end_delay"]			= conf.ptt.end_delay;
	data["ptt_out"]["reverse"] 				= conf.ptt.reverse;

	serializeJson(data, file);
	file.close();
}
