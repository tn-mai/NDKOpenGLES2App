/** @file AndroidAudio.cpp
*/
#include "Audio.h"
#include "../../Shared/File.h"
#include "android_native_app_glue.h"
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <vector>
#include <string>
#include <algorithm>
#include <stdint.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "SunnySideUp.Audio", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "SunnySideUp.Audio", __VA_ARGS__))

namespace Mai {

/// MP3 player.
struct AudioPlayer {
  SLObjectItf   player;
  SLmillisecond dur;
  SLmillisecond pos;
  SLPlayItf     playInterface;
  SLSeekItf     seekInterface;
  SLVolumeItf   volumeInterface;
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
  virtual void PlaySE(const char* id, float);
  virtual void PlayBGM(const char* filename, float);
  virtual void StopBGM(float);
  virtual void Clear();
  virtual void Update(float);

  void SetBGMVolume(float);
  void UpdateBGMVolume();

private:
  SLObjectItf  engineObject;
  SLEngineItf  engineInterface;
  SLObjectItf  mixObject;

  std::string  bgmFilename;
  bool  bgmIsPlay;
  float  bgmVolume;
  float  bgmFadeTime;
  float  bgmFadeTimer;

  AudioPlayer  bgmPlayer;
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

AudioImpl::AudioImpl()
  : engineObject(nullptr)
  , engineInterface(nullptr)
  , mixObject(nullptr)
  , bgmFilename()
  , bgmIsPlay(true)
  , bgmFadeTimer(0)
  , bgmPlayer{ nullptr }
{
}

/** Create audio player object.
  @sa DestroyMidiPlayer()
*/
bool CreateAudioPlayer(AudioPlayer& mp, SLEngineItf& eng, SLObjectItf& mix, const std::string& filename) {
  auto fd = FileSystem::GetFileDescriptor(filename.c_str());
  SLDataLocator_AndroidFD fileLoc = { SL_DATALOCATOR_ANDROIDFD, fd->fd, fd->start, fd->length };
  SLDataFormat_MIME fileFmt = { SL_DATAFORMAT_MIME, nullptr, SL_CONTAINERTYPE_UNSPECIFIED };
  SLDataSource fileSrc = { &fileLoc, &fileFmt };

  SLDataLocator_OutputMix audOutLoc = { SL_DATALOCATOR_OUTPUTMIX, mix };
  SLDataSink audOutSnk = { &audOutLoc, nullptr };

  const SLboolean required[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
  const SLInterfaceID iidArray[] = { SL_IID_PLAY, SL_IID_SEEK, SL_IID_VOLUME };

  SLresult result;
  result = (*eng)->CreateAudioPlayer(eng, &mp.player, &fileSrc, &audOutSnk, 3, iidArray, required);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to create CreateAudioPlayer:0x%lx", result);
    return false;
  }
  result = (*mp.player)->Realize(mp.player, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to realize AudioPlayer:0x%lx", result);
	return false;
  }
  result = (*mp.player)->GetInterface(mp.player, SL_IID_PLAY, &mp.playInterface);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get SL_IID_PLAY interface:0x%lx", result);
	return false;
  }
  result = (*mp.player)->GetInterface(mp.player, SL_IID_SEEK, &mp.seekInterface);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get SL_IID_SEEK interface:0x%lx", result);
	return false;
  }
  result = (*mp.player)->GetInterface(mp.player, SL_IID_VOLUME, &mp.volumeInterface);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get SL_IID_VOLUME interface:0x%lx", result);
	return false;
  }
  result = (*mp.playInterface)->GetDuration(mp.playInterface, &mp.dur);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get duration:0x%lx", result);
	return false;
  }

  (*mp.seekInterface)->SetLoop(mp.seekInterface, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
  (*mp.playInterface)->SetPlayState(mp.playInterface, SL_PLAYSTATE_PLAYING);

  LOGI("Create AudioPlayer");
  return true;
}

/** Destroy audio player object.
  @sa CreateMidiPlayer()
*/
void DestroyAudioPlayer(AudioPlayer& mp) {
  if (mp.player) {
	(*mp.player)->Destroy(mp.player);
	mp.player = nullptr;
	LOGI("Destroy AudioPlayer");
  }
}

/** Initialize sound controller object.
*/
bool AudioImpl::Initialize() {
  const SLInterfaceID engineMixIIDs[] = { SL_IID_ENGINE };
  const SLboolean engineMixReqs[] = { SL_BOOLEAN_TRUE };

  SLresult result = slCreateEngine(&engineObject, 0, nullptr, 1, engineMixIIDs, engineMixReqs);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed slCreateEngine:0x%lx", result);
	return false;
  }
  result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to realize AudioEngine:0x%lx", result);
	return false;
  }
  result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get SL_IID_ENGINE interface:0x%lx", result);
	return false;
  }

  const SLInterfaceID ids[] = {};
  const SLboolean req[] = {};
  result = (*engineInterface)->CreateOutputMix(engineInterface, &mixObject, 0, ids, req);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed CreateOutputMix:0x%lx", result);
	return false;
  }
  result = (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to realize OutputMix:0x%lx", result);
	return false;
  }

  LOGI("Create AudioEngine");
  return true;
}

void AudioImpl::Finalize() {
  DestroyAudioPlayer(bgmPlayer);
  if (mixObject) {
    (*mixObject)->Destroy(mixObject);
    mixObject = nullptr;
	LOGI("Destroy OutputMix");
  }
  if (engineObject) {
    (*engineObject)->Destroy(engineObject);
    engineObject = nullptr;
	LOGI("Destroy AudioEngine");
  }
}

bool AudioImpl::LoadSE(const char* id, const char* filename) {
  return false;
}

void AudioImpl::PrepareSE(const char* id)  {
}

void AudioImpl::PlaySE(const char* id, float volume) {
}

void AudioImpl::PlayBGM(const char* filename, float volume) {
  if (bgmPlayer.player) {
	if (bgmFilename == filename) {
	  (*bgmPlayer.seekInterface)->SetPosition(bgmPlayer.seekInterface, 0, SL_SEEKMODE_FAST);
	  (*bgmPlayer.playInterface)->SetPlayState(bgmPlayer.playInterface, SL_PLAYSTATE_STOPPED);
	  (*bgmPlayer.playInterface)->SetPlayState(bgmPlayer.playInterface, SL_PLAYSTATE_PLAYING);
	  SetBGMVolume(volume);
	  return;
	} else {
	  DestroyAudioPlayer(bgmPlayer);
	}
  }
  if (CreateAudioPlayer(bgmPlayer, engineInterface, mixObject, filename)) {
	bgmFilename = filename;
	bgmIsPlay = true;
	SetBGMVolume(volume);
  }
}

void AudioImpl::StopBGM(float fadeoutTimeSec) {
  if (bgmPlayer.player) {
	bgmFadeTime = bgmFadeTimer = std::max(0.0f ,fadeoutTimeSec);
	if (fadeoutTimeSec == 0.0f) {
	  bgmIsPlay = false;
	  (*bgmPlayer.playInterface)->SetPlayState(bgmPlayer.playInterface, SL_PLAYSTATE_PAUSED);
	  LOGI("StopBGM: %s", bgmFilename.c_str());
	}
  }
}

void AudioImpl::SetBGMVolume(float volume) {
  if (bgmPlayer.player) {
	bgmVolume = std::min(1.0f, std::max(0.0f, volume));
	bgmFadeTimer = bgmFadeTime = 0.0f;
	UpdateBGMVolume();
  }
}

void AudioImpl::UpdateBGMVolume() {
  if (bgmPlayer.player) {
	const float volume = bgmFadeTimer > 0.0f ? bgmVolume * bgmFadeTimer / bgmFadeTime : bgmVolume;
	const SLmillibel millibell = volume < 0.01f ? SL_MILLIBEL_MIN : static_cast<SLmillibel>(8000.0f * std::log10(volume));
	(*bgmPlayer.volumeInterface)->SetVolumeLevel(bgmPlayer.volumeInterface, millibell);
  }
}

void AudioImpl::Clear() {
}

void AudioImpl::Update(float tick) {
  if (bgmPlayer.player) {
	SLuint32 state;
	(*bgmPlayer.playInterface)->GetPlayState(bgmPlayer.playInterface, &state);
	switch (state) {
	case SL_PLAYSTATE_PLAYING:
	  if (bgmFadeTimer > 0.0f) {
		bgmFadeTimer = std::max(0.0f, bgmFadeTimer - tick);
		UpdateBGMVolume();
		if (bgmFadeTimer <= 0.0f) {
		  bgmIsPlay = false;
		  (*bgmPlayer.playInterface)->SetPlayState(bgmPlayer.playInterface, SL_PLAYSTATE_PAUSED);
		  LOGI("StopBGM: %s", bgmFilename.c_str());
		}
	  }
	  break;
	case SL_PLAYSTATE_PAUSED:
	  /* FALLTHROUGH */
	case SL_PLAYSTATE_STOPPED:
	  /* FALLTHROUGH */
	default:
	  break;
	}
  }
}

/** Create the sound engine object.

  @return the pointer to the interface of the sound engine implementation.
*/
AudioInterfacePtr CreateAudioEngine() {
  std::shared_ptr<AudioImpl> p(new AudioImpl());
  if (p) {
	if (!p->Initialize()) {
	  return nullptr;
	}
  }
  return p;
}

} // namespace Mai
