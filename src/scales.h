#include <Arduino.h>

#define SCALE_LENGTH 7

struct scale{
  String name;
  byte values[12];
  int length;
};

scale scales[] = {
    {"chromatic", {0,1,2,3,4,5,6,7,8,9,10,11}, 12},
    {"major", {0,2,4,5,7,8,11}, 7},
    {"minor", {0,2,3,5,7,8,11}, 7},
    {"major pentatonic", {0,2,4,7,9}, 5},
    {"minor pentatonic", {0,3,5,7,10}, 5},
    {"blues", {0,3,5,6,7,10}, 6},
    {"persian", {0,1,4,5,6,8,11}, 7}
};