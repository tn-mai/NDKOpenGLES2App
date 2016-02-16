/** @file Audio.h
*/
#ifndef MAI_COMMON_AUDIO_H_INCLUDED
#define MAI_COMMON_AUDIO_H_INCLUDED
#include <memory>

namespace Mai {

/** The sound manager.
*/
class AudioInterface {
public:
  virtual ~AudioInterface() {}
  virtual bool LoadSE(const char* id, const char* filename) = 0;
  virtual void PrepareSE(const char* id) = 0;
  virtual void PlaySE(const char* id) = 0;
  virtual void PlayBGM(const char* filename) = 0;
  virtual void StopBGM() = 0;
  virtual void Clear() = 0;
};

typedef std::shared_ptr<AudioInterface> AudioInterfacePtr;
AudioInterfacePtr CreateAudioEngine();

} // namespace Mai

#endif // MAI_COMMON_AUDIO_H_INCLUDED