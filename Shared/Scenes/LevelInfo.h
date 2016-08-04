/**
  @file LevelInfo.h
*/
#ifndef SSU_SCENE_LEVELINFO_H_INCLUDED
#define SSU_SCENE_LEVELINFO_H_INCLUDED
#include <cstdint>

namespace SunnySideUp {


/** The level information.
*/
struct CourseInfo {
  uint64_t seed;
  uint32_t startHeight; ///< The falling starting point for Eggman(2000-?).
  uint8_t hoursOfDay; ///< This affect for the lighting(0-23).
  uint8_t density; ///< The density of the obstacles.
  uint8_t difficulty; ///< This affect the allocation of the obstacles.
  uint8_t cloudage; ///< The amount of generating cloud(0:no cloud 7:maximum).
  uint8_t targetScale; ///< The scale of the target area.
  uint8_t description[32]; ///< The description of this level.

  static const uint8_t maxCloudage = 7;
};

/** The level information.
*/
struct LevelInfo {
  uint32_t level; ///< The identifier of this level.
  CourseInfo courses[4];
};

const CourseInfo& GetCourseInfo(uint32_t level, uint32_t courseNo);
uint32_t GetMaximumLevel();
uint32_t GetMaximumCourseNo(uint32_t level);

} // namespace SunnySideUp

#endif // SSU_SCENE_LEVELINFO_H_INCLUDED