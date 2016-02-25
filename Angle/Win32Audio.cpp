/** @file Win32Audio.cpp
*/
#include "../Shared/Audio.h"

namespace Mai {

/** The sound engine implementation.
*/
class AudioImpl: public AudioInterface {
public:
  AudioImpl();
  virtual ~AudioImpl() { Finalize(); }
  virtual bool Initialize();
  virtual void Finalize();
  virtual bool LoadSE(const char* id, const char* filename);
  virtual void PrepareSE(const char* id);
  virtual void PlaySE(const char* id, float);
  virtual void PlayBGM(const char* filename, float);
  virtual void StopBGM(float);
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

void AudioImpl::PrepareSE(const char* id)  {
}

void AudioImpl::PlaySE(const char* id, float) {
}

void AudioImpl::PlayBGM(const char* filename, float) {
}

void AudioImpl::StopBGM(float) {
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
