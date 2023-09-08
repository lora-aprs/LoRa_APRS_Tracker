
#include "power_management.h"

// cppcheck-suppress uninitMemberVar
PowerManagement::PowerManagement() {
}

// cppcheck-suppress unusedFunction
bool PowerManagement::begin(TwoWire &port) {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    bool result = axp.begin(port, AXP192_SLAVE_ADDRESS);
    if (!result) {
      axp.setDCDC1Voltage(3300);
    }
    return result;
  #endif
  #ifdef TTGO_T_Beam_V1_2
    bool result = PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, 21, 22);
    if (result) {
      PMU.disableDC2();
      PMU.disableDC3();
      PMU.disableDC4();
      PMU.disableDC5();
      PMU.disableALDO1();
      PMU.disableALDO4();
      PMU.disableBLDO1();
      PMU.disableBLDO2();
      PMU.disableDLDO1();
      PMU.disableDLDO2();

      PMU.setDC1Voltage(3300);
      PMU.enableDC1();
    }
    return result;
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::activateLoRa() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  #endif
  #ifdef TTGO_T_Beam_V1_2
    PMU.setALDO2Voltage(3300);
    PMU.enableALDO2();
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::deactivateLoRa() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
  #endif
  #ifdef TTGO_T_Beam_V1_2
    PMU.disableALDO2();
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::activateGPS() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  #endif
  #ifdef TTGO_T_Beam_V1_2
    PMU.setALDO3Voltage(3300);
    PMU.enableALDO3();
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::deactivateGPS() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
  #endif
  #ifdef TTGO_T_Beam_V1_2
    PMU.disableALDO3();
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::activateOLED() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::decativateOLED() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::disableChgLed() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setChgLEDMode(AXP20X_LED_OFF);
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::enableChgLed() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.setChgLEDMode(AXP20X_LED_LOW_LEVEL);
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::activateMeasurement() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.adc1Enable(AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, true);
  #endif
  #ifdef TTGO_T_Beam_V1_2
    PMU.enableBattDetection();
    PMU.enableVbusVoltageMeasure();
    PMU.enableBattVoltageMeasure();
    PMU.enableSystemVoltageMeasure();
  #endif
}

// cppcheck-suppress unusedFunction
void PowerManagement::deactivateMeasurement() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    axp.adc1Enable(AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, false);
  #endif
}

// cppcheck-suppress unusedFunction
double PowerManagement::getBatteryVoltage() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    return axp.getBattVoltage() / 1000.0;
  #endif
  #ifdef TTGO_T_Beam_V1_2
    return PMU.getBattVoltage() / 1000.0;
  #endif
}

// cppcheck-suppress unusedFunction
double PowerManagement::getBatteryChargeDischargeCurrent() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    if (axp.isChargeing()) {
      return axp.getBattChargeCurrent();
    }
    return -1.0 * axp.getBattDischargeCurrent();
  #endif
  return 0;
}

bool PowerManagement::isBatteryConnect() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    return axp.isBatteryConnect();
  #endif
  #ifdef TTGO_T_Beam_V1_2
    return PMU.isBatteryConnect();
  #endif
}

bool PowerManagement::isChargeing() {
  #if defined(TTGO_T_Beam_V0_7) || defined(TTGO_T_Beam_V1_0)
    return axp.isChargeing();
  #endif
  #ifdef TTGO_T_Beam_V1_2
    return PMU.isCharging();
  #endif
}
