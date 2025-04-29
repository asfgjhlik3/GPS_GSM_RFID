#include "stops.h"
# include "math.h"

struct Barangay {
    const char* name;
    const char* busStopName;
    float lat;
    float lon;
    float proximityThreshold;
  };
  Barangay barangays[] = {
    {"Robinsons Supermarket", "ROBINSONS", 8.21838, 124.24034, 50.0},
    {"MSU-IIT", "MSU-IIT", 8.2109886, 124.2242461, 50.0},
    {"Capitol College", "CAPITOL", 8.22249, 124.24059, 50.0},
    {"IBJT North Tambo", "IBJT NORTH TAMBO", 8.24583, 124.25528, 50},
    {"Tambo Overpass", "TAMBO OVERPASS", 8.244840, 124.255514, 50.0},
    {"SSS Iligan Office", "SSS ILIGAN OFFICE", 8.241817, 124.248054, 50.0},
    {"Franciscan", "Franciscan", 8.243800, 124.250455, 50.0},
    {"Kingsway Inn", "KINGSWAY INN", 8.23303, 124.24169, 50.0},
    {"Solana/SBG Tibanga", "SOLANA /SBGTIBANGA", 8.23692, 124.24383, 50.0},
    {"Gaisano Mall", "GAISANO MALL", 8.23095, 124.24136,50.0},
    {"Metrobank", "METROBANK", 8.22984, 124.24083, 50.0},
    {"Jollibee Aguinaldo", "JOLLIBEE AGUINALDO", 8.22792, 124.24047, 50.0},
    {"ICNHS", "ICNHS", 8.22579, 124.23994, 50.0},
    {"Petron Tubod", "PETRON TUBOD", 8.21654, 124.23866, 50.0},
    {"Mercy Junction", "MERCY JUNCTION", 8.21537, 124.23146, 50.0},
    {"Petron Camague", "PETRON CAMAGUE", 8.21254, 124.22707, 50.0},
    {"IMDI Overpass", "IMDI OVERPASSS", 8.21063, 124.22201, 50.0},
    {"Shell Tominobo", "SHELL TOMINOBO", 8.20969, 124.21879, 50.0},
    {"Tabay 1,2", "TABAY 1,2", 8.20485, 124.22463, 50.0},
    {"IBJT South-NSC", "IBJT SOUTH-NSC", 8.20744, 124.21641, 50.0},
    {"Bayanihan Village, Brgy, Sta. Elena", "Bayanihan", 8.19636, 124.22690, 50.0}
  };
  const int barangayCount = sizeof(barangays) / sizeof(barangays[0]);
  float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
    float dLat = (lat2 - lat1) * (M_PI / 180.0);
    float dLon = (lon2 - lon1) * (M_PI / 180.0);
  
    float a = sin(dLat / 2) * sin(dLat / 2) +
              cos(lat1 * (M_PI / 180.0)) * cos(lat2 * (M_PI / 180.0)) *
              sin(dLon / 2) * sin(dLon / 2);
  
    float c = 2 * atan2(sqrt(a), sqrt(1 - a));
    float earthRadiusKm = 6371.0;
    return earthRadiusKm * c;
}

  const char* getNearestStopFromGPS(float currentLat, float currentLon) {
    const float MAX_DISTANCE = 0.005;  // Approx ~500m depending on latitude
    float closestDistance = MAX_DISTANCE;
    const char* nearestStop = "UNKNOWN";
  
    for (int i = 0; i < barangayCount; i++) {
      float distance = calculateDistance(currentLat, currentLon, barangays[i].lat, barangays[i].lon);
      if (distance < closestDistance) {
        closestDistance = distance;
        nearestStop = barangays[i].busStopName;
      }
    }
  
    return nearestStop;
  }

  