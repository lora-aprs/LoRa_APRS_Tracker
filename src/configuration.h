#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <list>

#include <Arduino.h>
#ifndef CPPCHECK
#include <ArduinoJson.h>
#endif

class Configuration
{
public:
	class Wifi
	{
	public:
		class AP
		{
		public:
			String SSID;
			String password;
		};

		Wifi() : active(false) {}

		bool active;
		std::list<AP> APs;
	};

	class Beacon
	{
	public:
		Beacon() : message("LoRa Tracker, Info: github.com/peterus/LoRa_APRS_Tracker") {}

		String message;
		int  timeout = 1;
        String symbol;
        String overlay; 
		
	};

	class Smart_Beacon
	{
		public: 
		Smart_Beacon() :  active(true), turn_min(25), slow_rate(300), slow_speed(10), fast_rate(60), fast_speed(100), min_tx_dist(100), min_bcn(5) {}
		
		bool active;
        int turn_min;
		int slow_rate;
		int slow_speed;
		int fast_rate;
		int fast_speed;
		int min_tx_dist;
		int min_bcn;
	};

	class APRS_IS
	{
	public:
		APRS_IS() : active(false), server("euro.aprs2.net"), port(14580), beacon(true), beaconTimeout(15) {}

		bool active;
		String password;
		String server;
		int port;
		bool beacon;
		int beaconTimeout;
	};

	class Digi
	{
	public:
		Digi() : active(false), forwardTimeout(5), beacon(true), beaconTimeout(30) {}

		bool active;
		int forwardTimeout;
		bool beacon;
		int beaconTimeout;
	};

	class LoRa
	{
	public:
		LoRa() : frequencyRx(433775E3), frequencyTx(433775E3), power(20), spreadingFactor(12), signalBandwidth(125000), codingRate4(5) {}

		long frequencyRx;
		long frequencyTx;
		int power;
		int spreadingFactor;
		long signalBandwidth;
		int codingRate4;
	};

	class Display
	{
	public:
		Display() : alwaysOn(true), timeout(10), overwritePin(0) {}

		bool alwaysOn;
		int timeout;
		int overwritePin;
	};

	class Ftp
	{
	public:
		class User
		{
		public:
			String name;
			String password;
		};

		Ftp() : active(false) {}

		bool active;
		std::list<User> users;
	};

	Configuration() : callsign("NOCALL-10") {};

	String callsign;
	Wifi wifi;
	Beacon beacon;
	Smart_Beacon smart_beacon;
	APRS_IS aprs_is;
	Digi digi;
	LoRa lora;
	Display display;
	Ftp ftp;
	bool debug;
};

class ConfigurationManagement
{
public:
	explicit ConfigurationManagement(String FilePath);

	Configuration readConfiguration();
	void writeConfiguration(Configuration conf);

private:
	const String mFilePath;
};

#endif
