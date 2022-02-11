#include "semanticVersion.hpp"

SemanticVersion::SemanticVersion() {}

void SemanticVersion::init(int version) {
  init(0, 0, version);
}

void SemanticVersion::init(String version) {
  std::vector<int> vVersion = formatVersion(version);
  init(vVersion[0], vVersion[1], vVersion[2]);
}

void SemanticVersion::init(int majorVersion, int minorVersion, int patchVersion) {
  this->majorVersion = majorVersion;
  this->minorVersion = minorVersion;
  this->patchVersion = patchVersion;
}

bool SemanticVersion::isGreater(String newVersion) {
  std::vector<int> vVersion = formatVersion(newVersion);
  if (majorVersion < vVersion[0]) return true;
  if (majorVersion == vVersion[0]) {
    if (minorVersion < vVersion[1]) return true;
    if (minorVersion == vVersion[1])
      if (patchVersion < vVersion[2]) return true;
  }
  return false;
}

std::vector<int> SemanticVersion::formatVersion(String version) {
  std::vector<int> vVersion{0, 0, 0};
  bool swAdd = false;

  for (int i=2, index=version.length(); i >= 0 && index > 0; i--) {
    int auxIndex = version.lastIndexOf('.', index-1);
    if (auxIndex < 0 && !swAdd) {
      auxIndex = 0;
      swAdd = true;
    } else auxIndex++;
    if (auxIndex >= 0) {
      String auxVersion = version.substring(auxIndex, index);
      vVersion[i] = atoi(auxVersion.c_str());
      index = auxIndex-1;
    }
  }
  return vVersion;
}