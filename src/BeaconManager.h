#ifndef BEACON_MANAGER_H_
#define BEACON_MANAGER_H_

#include "configuration.h"

class BeaconManager {
public:
  BeaconManager();

  void loadConfig(const std::list<Configuration::Beacon> &beacon_config);

  std::list<Configuration::Beacon>::iterator getCurrentBeaconConfig() const;
  void                                       loadNextBeacon();

private:
  std::list<Configuration::Beacon>           _beacon_config;
  std::list<Configuration::Beacon>::iterator _currentBeaconConfig;
};

#endif
