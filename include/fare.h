#ifndef FARE_MATRIX_H
#define FARE_MATRIX_H

constexpr int NUM_STOPS = 21;
extern const float fareMatrix_Bayanihan_to_IBJT[NUM_STOPS][NUM_STOPS];
extern const float fareMatrix_IBJT_to_Bayanihan[NUM_STOPS][NUM_STOPS];

// Returns the fare based on direction and index of start/end stops
float getFare(int fromIndex, int toIndex, bool isReverse = false);
float calculateFare(const char* tapInStop, const char* tapOutStop);

#endif