#ifndef semanticVersion_hpp
#define semanticVersion_hpp

#include "Arduino.h"
#include <iostream>
#include <vector>

class SemanticVersion {
  public:
    SemanticVersion();
    void init(int version);
    void init(String version);
    bool isGreater(String newVersion);

  private:
    int majorVersion, minorVersion, patchVersion;
    void init(int majorVersion, int minorVersion, int patchVersion);
    std::vector<int> formatVersion(String version);
};

#endif