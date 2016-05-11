/**
  @file LevelInfo.cpp
*/
#include "LevelInfo.h"
#include <algorithm>

namespace SunnySideUp {

const uint8_t noon = 10;
const uint8_t sunset = 17;
const uint8_t night = 0;

static const LevelInfo infoArray[] = {
  { 1, 2000, noon,   1, 1, 1, "Tutorial" },
  { 2, 2500, noon,   2, 1, 3, "Newbie Diver" },
  { 3, 3000, sunset, 3, 2, 3, "Sunset Cooking" },
  { 4, 3000, night,  3, 2, 2, "Cooking At Night" },
  { 5, 4000, noon,   2, 2, 7, "Dive In Deep Sky" },
  { 6, 4000, sunset, 3, 3, 5, "What Is Your Number?" },
  { 7, 4000, night,  3, 3, 4, "Tasty Egg" },
  { 8, 4000, noon,   4, 4, 5, "Truth Of The Cooking" },
};

const LevelInfo& GetLevelInfo(uint32_t level) {
  level = std::min(level, GetMaximumLevel());
  return infoArray[level];
}

uint32_t GetMaximumLevel() {
  return sizeof(infoArray) / sizeof(infoArray[0]) - 1;
}

} // namespace SunnySideUp