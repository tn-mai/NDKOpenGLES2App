/** @file Win32Audio.cpp
*/
#include "../Shared/Audio.h"

namespace Mai {

class AudioCueImpl : public AudioCue {
public:
  AudioCueImpl() {}
  virtual ~AudioCueImpl() {}
  virtual bool IsPrepared() const { return true; }
  virtual void Play(float) {}
};

  /** The sound engine implementation.
*/
class AudioImpl: public AudioInterface {
public:
  AudioImpl();
  virtual ~AudioImpl() { Finalize(); }
  virtual bool Initialize();
  virtual void Finalize();
  virtual bool LoadSE(const char* id, const char* filename);
  virtual AudioCuePtr PrepareSE(const char* id);
  virtual void PlaySE(const char* id, float);
  virtual void PlayBGM(const char* filename, float);
  virtual void StopBGM(float);
  virtual void SetBGMLoopFlag(bool);
  virtual void Clear();
  virtual void Update(float);
};

AudioImpl::AudioImpl() {
}
/** Initialize sound controller object.
*/
bool AudioImpl::Initialize() {
  return true;
}

void AudioImpl::Finalize() {
}

bool AudioImpl::LoadSE(const char* id, const char* filename) {
  return false;
}

AudioCuePtr AudioImpl::PrepareSE(const char* id)  {
  return AudioCuePtr(new AudioCueImpl);
}

void AudioImpl::PlaySE(const char* id, float) {
}

void AudioImpl::PlayBGM(const char* filename, float) {
}

void AudioImpl::StopBGM(float) {
}

void AudioImpl::SetBGMLoopFlag(bool) {
}

void AudioImpl::Clear() {
}

void AudioImpl::Update(float) {
}

/** Create the sound engine object.

  @return the pointer to the interface of the sound engine implementation.
*/
AudioInterfacePtr CreateAudioEngine() {
  return AudioInterfacePtr(new AudioImpl());
}

} // namespace Mai
