/** @file Audio.h
*/
#ifndef MAI_COMMON_AUDIO_H_INCLUDED
#define MAI_COMMON_AUDIO_H_INCLUDED
#include <memory>

namespace Mai {

/** The sound cue.
*/
class AudioCue {
public:
  virtual ~AudioCue() {}
  virtual bool IsPrepared() const = 0;
  virtual void Play(float volume = 1.0f) = 0;
};
typedef std::shared_ptr<AudioCue> AudioCuePtr;

/** The sound manager.
*/
class AudioInterface {
public:
  virtual ~AudioInterface() {}
  virtual bool LoadSE(const char* id, const char* filename) = 0;
  virtual AudioCuePtr PrepareSE(const char* id) = 0;
  virtual void PlaySE(const char* id, float volume = 0.5f) = 0;
  virtual void PlayBGM(const char* f, float volume = 0.5f) = 0;
  virtual void StopBGM(float fadeoutTimeSec = 0.5f) = 0;

  /**
  * Set the loop flag to current BGM.
  *
  * @param loop  - true: Enable looping.
  *              - false: Disable looping.
  *
  * The loop flag is initialized 'true' when PlayBGM() was called.
  */
  virtual void SetBGMLoopFlag(bool loop) = 0;
  virtual void Clear() = 0;
  virtual void Update(float tick) = 0;
};

typedef std::shared_ptr<AudioInterface> AudioInterfacePtr;
AudioInterfacePtr CreateAudioEngine();

} // namespace Mai

#endif // MAI_COMMON_AUDIO_H_INCLUDED
