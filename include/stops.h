#ifndef STOPS_H
#define STOPS_H

struct Barangay {
  const char* name;
  const char* busStopName;
  float lat;
  float lon;
  float proximityThreshold;
  bool isBase;
};

extern const int barangayCount;
extern const Barangay barangays[];

const char* getNearestStopFromGPS(float currentLat, float currentLon);

#endif
