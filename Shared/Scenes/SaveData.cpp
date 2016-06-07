/**
  @file SaveData.cpp

  The save data format(little endian):
  int32_t record count
  [
    int64_t record time
	int16_t level
	int16_t  year
	int8_t  month
	int8_t  day
	int8_t  hour
	int8_t  minit
	int8_t  second
	] x (record count)
*/
#include "SaveData.h"
#include <vector>
#include <algorithm>
#include <limits>
#include <ctime>

#ifdef __ANDROID__
#include "android_native_app_glue.h"
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "SunnySideUp.SaveData", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "SunnySideUp.SaveData", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

namespace SunnySideUp {

namespace SaveData {

/// Save data filename.
static const char recordFilename[] = "SunnySideUpRecords";

/// Best record of each level.
std::vector<Record>  bestRecords;

/** Get a value from the byte stream.

  @param p     The pointer to the byte stream.
  @param size  The data size(unit:byte).

  @return A value maked from the byte stream.

  Note that 'p' is advanced 'size' byte.
*/
int64_t GetValue(const uint8_t*& p, size_t size) {
  int64_t result = 0;
  int64_t scale = 1;
  for (size_t i = 0; i < size; ++i, ++p) {
	result += *p * scale;
	scale *= 0x100LL;
  }
  return result;
}

/** Set a value to the byte stream.

  @param p     The pointer to the byte stream.
  @param size  The data size(unit:byte).
  @param value A value that will be storing to 'p'.

  Note that 'p' is advanced 'size' byte.
*/
void SetValue(uint8_t*& p, size_t size, int64_t value) {
  for (size_t i = 0; i < size; ++i, ++p) {
	*p = value % 0xffLL;
	value /= 0x100LL;
  }
}

/** Write the record to file.

  @param  window  The window object.

  @sa PrepareSaveData()
*/
void WriteSaveData(const Mai::Window& window) {
  std::vector<uint8_t>  buf;
  buf.resize(bestRecords.size() * 17 + 4);
  uint8_t* p = buf.data();
  SetValue(p, 4, bestRecords.size());
  for (const auto& record : bestRecords) {
	SetValue(p, 8, record.time);
	SetValue(p, 2, record.level);
	SetValue(p, 2, record.year);
	SetValue(p, 1, record.month);
	SetValue(p, 1, record.day);
	SetValue(p, 1, record.hour);
	SetValue(p, 1, record.minit);
	SetValue(p, 1, record.second);
  }
  window.SaveUserFile(recordFilename, buf.data(), buf.size());
}

/** Load the save data.

  @param  window  The window object.

  @retval true  Success to load the save data.
  @retval false Failure. probably, play at first time.
*/
bool PrepareSaveData(const Mai::Window& window) {
  const size_t size = window.GetUserFileSize(recordFilename);
  if (size == 0) {
	return false;
  }
  std::vector<uint8_t> buf;
  buf.resize(size);
  if (!window.LoadUserFile(recordFilename, buf.data(), size)) {
	LOGI("No save data: %s", recordFilename);
	return false;
  }

  bestRecords.clear();
  const uint8_t* p = buf.data();
  const size_t recordCount = static_cast<size_t>(GetValue(p, 4));
  LOGI("recordCount: %d", recordCount);
  if (recordCount) {
	bestRecords.resize(recordCount);
	for (size_t i = 0; i < recordCount; ++i) {
	  Record& record = bestRecords[i];
	  record.time = static_cast<int64_t>(GetValue(p, 8));
	  record.level = static_cast<int16_t>(GetValue(p, 2));
	  record.year = static_cast<int16_t>(GetValue(p, 2));
	  record.month = static_cast<int8_t>(GetValue(p, 1));
	  record.day = static_cast<int8_t>(GetValue(p, 1));
	  record.hour = static_cast<int8_t>(GetValue(p, 1));
	  record.minit = static_cast<int8_t>(GetValue(p, 1));
	  record.second = static_cast<int8_t>(GetValue(p, 1));
	  LOGI("lv:%d time:%03.3f %d/%02d/%02d %02d:%02d", record.level, static_cast<float>(record.time) / 1000.0f, record.year + 1900, record.month + 1, record.day, record.hour, record.minit);
	}
	std::sort(bestRecords.begin(), bestRecords.end(), [](const Record& lhs, const Record& rhs) { return lhs.level < rhs.level; });
	const auto end = bestRecords.end();
	auto itrStore = bestRecords.begin();
	for (auto itr = itrStore + 1; itr != end; ++itr) {
	  if (itr->level != itrStore->level) {
		++itrStore;
		if (itrStore != itr) {
		  *itrStore = *itr;
		}
		continue;
	  }
	  if (itr->time < itrStore->time) {
		*itrStore = *itr;
	  }
	}
	bestRecords.erase(itrStore + 1, bestRecords.end());
  }
  return true;
}

/** Get a best record.

  @param level  The target level.

  @return Ff the record exist, boost::optional contain a best record for the level.
          Otherwise boost::none.
*/
boost::optional<Record> GetBestRecord(int level) {
  const auto itr = std::find_if(bestRecords.begin(), bestRecords.end(), [level](const Record& e) {return e.level == level; });
  if (itr == bestRecords.end()) {
	return boost::none;
  }
  return *itr;
}

/** Set new record.

  @param window      The window object.
  @param level       The target level.
  @param recordTime  A new time of the level.

  @retval true   The best record was overwritten by the 'recordTime'.
  @retval false  'recordTime' did not exceed the best record.
*/
bool SetNewRecord(const Mai::Window& window, int level, int64_t recordTime) {
  time_t currentTime;
  time(&currentTime);
  const struct tm* pGMT = gmtime(&currentTime);
  if (!pGMT) {
	LOGE("ERROR in SetNewRecord: gmtime failed(time:%ld)", currentTime);
	return false;
  }
  Record record;
  record.time = recordTime;
  record.level = static_cast<int16_t>(level);
  record.year = static_cast<int16_t>(pGMT->tm_year);
  record.month = static_cast<int8_t>(pGMT->tm_mon);
  record.day = static_cast<int8_t>(pGMT->tm_mday);
  record.hour = static_cast<int8_t>(pGMT->tm_hour);
  record.minit = static_cast<int8_t>(pGMT->tm_min);
  record.second = static_cast<int8_t>(pGMT->tm_sec);

  const auto itr = std::find_if(bestRecords.begin(), bestRecords.end(), [level](const Record& e) {return e.level == level; });
  if (itr == bestRecords.end()) {
	LOGI("Add new record:%lld", recordTime);
	bestRecords.push_back(record);
	WriteSaveData(window);
	return true;
  }
  if (record.time < itr->time) {
	LOGI("Update by new record:%lld -> %lld", itr->time, recordTime);
	*itr = record;
	WriteSaveData(window);
	return true;
  }
  return false;
}

} // namespace SaveData;

} // namespace SunnySideUp
