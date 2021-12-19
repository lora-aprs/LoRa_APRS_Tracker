#include "BeaconManager.h"

BeaconManager::BeaconManager() : _currentBeaconConfig(_beacon_config.end()) {
}

// cppcheck-suppress unusedFunction
void BeaconManager::loadConfig(const std::list<Configuration::Beacon> &beacon_config) {
  _beacon_config       = beacon_config;
  _currentBeaconConfig = _beacon_config.begin();
}

// cppcheck-suppress unusedFunction
std::list<Configuration::Beacon>::iterator BeaconManager::getCurrentBeaconConfig() const {
  return _currentBeaconConfig;
}

// cppcheck-suppress unusedFunction
void BeaconManager::loadNextBeacon() {
  ++_currentBeaconConfig;
  if (_currentBeaconConfig == _beacon_config.end()) {
    _currentBeaconConfig = _beacon_config.begin();
  }
}
