#ifndef POWER_MANAGEMENT_H_
#define POWER_MANAGEMENT_H_

#include <Arduino.h>

#if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
  #include <axp20x.h>
#endif
#ifdef TTGO_T_Beam_V1_2
  #define XPOWERS_CHIP_AXP2101
  #include <XPowersLib.h>
#endif




class PowerManagement {
public:
  PowerManagement();
  bool begin(TwoWire &port);

  void activateLoRa();
  void deactivateLoRa();

  void activateGPS();
  void deactivateGPS();

  void activateOLED();
  void decativateOLED();

  void enableChgLed();
  void disableChgLed();

  void activateMeasurement();
  void deactivateMeasurement();

  double getBatteryVoltage();
  double getBatteryChargeDischargeCurrent();

  bool isBatteryConnect();
  bool isChargeing();

private:
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    AXP20X_Class axp;
  #endif
  #ifdef TTGO_T_Beam_V1_2
  XPowersPMU PMU;
  #endif
};

#endif
