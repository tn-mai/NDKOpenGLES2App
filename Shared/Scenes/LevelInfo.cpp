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
  {
	1,
	{
	  { 101, 2000, noon,   1, 1, 1, 20, "Tutorial 1" },
	  { 118, 2000, noon,   1, 1, 1, 20, "Tutorial 2" },
	  { 130, 2500, sunset, 1, 1, 1, 15, "Tutorial 3" },
	}
  },
  {
	2,
	{
	  { 201, 2500, noon,   2, 1, 3, 15, "Newbie Diver 1" },
	  { 202, 2500, noon,   2, 1, 3, 15, "Newbie Diver 2" },
	  { 203, 3000, noon,   2, 1, 3, 15, "Newbie Diver 3" },
	}
  },
  {
	3,
	{
	  { 301, 3000, sunset, 3, 2, 3, 15, "Sunset Cooking 1" },
	  { 302, 3000, sunset, 3, 2, 3, 15, "Sunset Cooking 2" },
	  { 303, 3000, night,  3, 2, 3, 15, "Sunset Cooking 3" },
	}
  },
  {
	4,
	{
	  { 401, 3000, night,  3, 2, 2, 15, "Cooking At Night 1" },
	  { 402, 3000, night,  3, 2, 2, 15, "Cooking At Night 2" },
	  { 403, 3000, night,  3, 2, 2, 10, "Cooking At Night 3" },
	}
  },
  {
	5,
	{
	  { 501, 4000, noon,   2, 2, 7, 10, "Dive In Deep Sky 1" },
	  { 502, 4000, noon,   2, 2, 7, 10, "Dive In Deep Sky 2" },
	  { 503, 4000, sunset, 2, 2, 7, 10, "Dive In Deep Sky 3" },
	}
  },
  {
	6,
	{
	  { 601, 4000, sunset, 3, 3, 5, 10, "What Is Your Number? 1" },
	  { 602, 4000, sunset, 3, 3, 5, 10, "What Is Your Number? 2" },
	  { 603, 4000, night,  3, 3, 5, 10, "What Is Your Number? 3" },
	}
  },
  {
	7,
	{
	  { 701, 4000, night,  3, 3, 4, 10, "Tasty Egg 1" },
	  { 702, 4000, night,  3, 3, 4, 10, "Tasty Egg 2" },
	  { 703, 4000, night,  3, 3, 4, 10, "Tasty Egg 3" },
	}
  },
  {
	8,
	{
	  { 801, 4000, noon,   4, 4, 5, 5, "Truth Of The Cooking 1" },
	  { 802, 4000, sunset, 4, 4, 5, 5, "Truth Of The Cooking 2" },
	  { 803, 4000, night,  4, 4, 5, 5, "Truth Of The Cooking 3" },
	}
  },
};

const CourseInfo& GetCourseInfo(uint32_t level, uint32_t courseNo) {
  level = std::min(level, GetMaximumLevel());
  return infoArray[level].courses[courseNo];
}

uint32_t GetMaximumLevel() {
  return sizeof(infoArray) / sizeof(infoArray[0]) - 1;
}

uint32_t GetMaximumCourseNo(uint32_t level) {
  return 2;
}

} // namespace SunnySideUp