/** @file AndroidAudio.cpp
*/
#include "Audio.h"
#include "../../Shared/File.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdint.h>

namespace Mai {

/// MIDI
struct MIDIPlayer {
  SLObjectItf   player;
  SLmillisecond dur;
  SLmillisecond pos;
  SLPlayItf     playInterface;
  SLSeekItf     seekInterface;
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
  virtual void PrepareSE(const char* id);
  virtual void PlaySE(const char* id);
  virtual void PlayBGM(const char* filename);
  virtual void StopBGM();
  virtual void Clear();

private:
  SLObjectItf  engineObject;
  SLEngineItf  engineInterface;
  SLObjectItf  mixObject;
  SLVolumeItf  volumeInterface;

  std::string  bgmFilename;

  MIDIPlayer   midiPlayer;
};

struct SampleBuffer {
    std::vector<uint8_t> buf;       // audio sample container
    uint32_t cap;       // buffer capacity in byte
    uint32_t size;      // audio sample size (n buf) in byte
};
typedef std::vector<SampleBuffer> SampleBufferList;

SampleBufferList allocateSampleBufs(uint32_t count, uint32_t sizeInByte) {
  count = std::max(2U, count);
  sizeInByte = std::max(1U, sizeInByte);

  SampleBufferList bufs(count);

  const uint32_t allocSize = (sizeInByte + 3) & ~3;   // padding to 4 bytes aligned
  for(uint32_t i = 0; i < count; ++i) {
      bufs[i].buf.resize(allocSize);
      bufs[i].cap = sizeInByte;
      bufs[i].size = 0;        //0 data in it
  }
  return bufs;
}

AudioImpl::AudioImpl() {
}

/** Create MIDI player object.
  @sa DestroyMidiPlayer()
*/
bool CreateMidiPlayer(MIDIPlayer& mp, SLEngineItf eng, SLObjectItf mix) {
  auto fd = FileSystem::GetFileDescriptor("Midis/wind.mid");
  SLDataLocator_AndroidFD fileLoc = { SL_DATALOCATOR_ANDROIDFD, fd->fd, fd->start, fd->length };
  SLDataFormat_MIME fileFmt = { SL_DATAFORMAT_MIME, (SLchar*)"audio/x-midi", SL_CONTAINERTYPE_SMF };

  const SLboolean required[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_FALSE };
  const SLInterfaceID iidArray[] = { SL_IID_SEEK, SL_IID_PLAY, SL_IID_VOLUME };

  SLDataSource fileSrc = { &fileFmt, &fileLoc };
  SLDataLocator_OutputMix audOutLoc = { SL_DATALOCATOR_OUTPUTMIX, mix };
  SLDataSink    audOutSnk = { &audOutLoc, nullptr };

  SLresult result = (*eng)->CreateMidiPlayer(eng, &mp.player, &fileSrc, nullptr, &audOutSnk, nullptr, nullptr, 1, iidArray, required);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*mp.player)->Realize(mp.player, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*mp.player)->GetInterface(mp.player, SL_IID_PLAY, &mp.playInterface);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*mp.player)->GetInterface(mp.player, SL_IID_SEEK, &mp.seekInterface);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*mp.playInterface)->GetDuration(mp.playInterface, &mp.dur);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }

  (*mp.seekInterface)->SetLoop(mp.seekInterface, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
  (*mp.playInterface)->SetPlayState(mp.playInterface, SL_PLAYSTATE_PLAYING);

  return true;
}

/** Destroy MIDI player object.
  @sa CreateMidiPlayer()
*/
void DestroyMidiPlayer(MIDIPlayer& mp) {
  if (mp.player) {
	(*mp.player)->Destroy(mp.player);
	mp.player = nullptr;
  }
}

/** Initialize sound controller object.
*/
bool AudioImpl::Initialize() {
  SLresult result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }

  const SLInterfaceID ids[1] = {SL_IID_VOLUME};
  const SLboolean req[1] = {SL_BOOLEAN_TRUE};
  result = (*engineInterface)->CreateOutputMix(engineInterface, &mixObject, 1, ids, req);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }
  result = (*mixObject)->GetInterface(mixObject, SL_IID_VOLUME, &volumeInterface);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }

  return true;
}

void AudioImpl::Finalize() {
  DestroyMidiPlayer(midiPlayer);
  if (mixObject) {
    (*mixObject)->Destroy(mixObject);
    mixObject = nullptr;
  }
  if (engineObject) {
    (*engineObject)->Destroy(engineObject);
    engineObject = nullptr;
  }
}

bool AudioImpl::LoadSE(const char* id, const char* filename) {
  return false;
}

void AudioImpl::PrepareSE(const char* id)  {
}

void AudioImpl::PlaySE(const char* id) {
}

void AudioImpl::PlayBGM(const char* filename) {
  if (!midiPlayer.player || bgmFilename != filename) {
    if (CreateMidiPlayer(midiPlayer, engineInterface, mixObject)) {
      bgmFilename = filename;
    }
  }
}

void AudioImpl::StopBGM() {
  if (midiPlayer.playInterface) {
    (*midiPlayer.playInterface)->SetPlayState(midiPlayer.playInterface, SL_PLAYSTATE_STOPPED);
  }
}

void AudioImpl::Clear() {
}

/** Create the sound engine object.

  @return the pointer to the interface of the sound engine implementation.
*/
AudioInterfacePtr CreateAudioEngine() {
  return AudioInterfacePtr(new AudioImpl());
}

} // namespace Mai
