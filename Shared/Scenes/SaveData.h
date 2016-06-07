/**
  @file SaveData.h
*/
#ifndef SSU_SAVEDATA_H_INCLUDED
#define SSU_SAVEDATA_H_INCLUDED
#include "../Window.h"
#include <boost/optional.hpp>
#include <cstdint>

namespace SunnySideUp {

/// SaveData namespace.
namespace SaveData {

struct Record {
  int64_t  time;
  int16_t  level;

  // GMT when achieved this record.
  int16_t  year;
  int8_t  month;
  int8_t  day;
  int8_t  hour;
  int8_t  minit;
  int8_t  second;
};

bool PrepareSaveData(const Mai::Window&);
boost::optional<Record> GetBestRecord(int level);
bool SetNewRecord(const Mai::Window&, int level, int64_t time);
void DeleteAll(const Mai::Window&);

} // namespace SaveData

} // namespace SunnySideup

#endif // SSU_SAVEDATA_H_INCLUDED