/** @file AndroidAudio.cpp
*/
#include "Audio.h"
#include "../../Shared/File.h"
#include "android_native_app_glue.h"
#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <algorithm>
#include <stdint.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "SunnySideUp.Audio", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "SunnySideUp.Audio", __VA_ARGS__))

namespace Mai {

/** Get the millibel value of volume from the floating point volume.

  @param volume  the floating point volume. the minimum is 0.0f. the maximum is 1.0f.

  @return the millibel volume that is converted from the floating point volume.
*/
SLmillibel GetMillibelVolume(float volume) {
  return volume < 0.01f ? SL_MILLIBEL_MIN : static_cast<SLmillibel>(8000.0f * std::log10(volume));
}

struct WaveSource {
  std::string id;
  SLDataFormat_PCM pcm;
  std::vector<uint8_t> data;
  bool operator<(const WaveSource& n) const { return id < n.id; }
};
  
/// MP3 player.
struct AudioPlayer {
  AudioPlayer() : player(nullptr) {}

  SLObjectItf   player;
  SLmillisecond dur;
  SLmillisecond pos;
  SLPlayItf     playInterface;
  SLSeekItf     seekInterface;
  SLVolumeItf   volumeInterface;
};

struct BufferQueueAudioPlayer {
  BufferQueueAudioPlayer() : refCount(0), player(nullptr), pSource(nullptr) {}

  int refCount;

  SLObjectItf   player;
  SLmillisecond dur;
  SLmillisecond pos;
  SLPlayItf     playInterface;
  SLVolumeItf   volumeInterface;
  SLAndroidSimpleBufferQueueItf bufferQueueInterface; ///< used by SE player.

  const WaveSource* pSource; ///< the player can play new sound if nullptr.
};

/** The BufferQueue callback function.
*/
void BufferQueueCallback(SLAndroidSimpleBufferQueueItf caller, void* pContext) {
  BufferQueueAudioPlayer* p = static_cast<BufferQueueAudioPlayer*>(pContext);
  (*p->playInterface)->SetPlayState(p->playInterface, SL_PLAYSTATE_STOPPED);
  if (p->pSource) {
	LOGI("BufferQueueCallback: %s", p->pSource->id.c_str());
  } else {
	LOGI("BufferQueueCallback: (empty)");
  }
}

class AudioCueImpl : public AudioCue {
public:
  AudioCueImpl(BufferQueueAudioPlayer* p) : pPlayer(p) {
	if (pPlayer) {
	  ++pPlayer->refCount;
	}
  }
  virtual ~AudioCueImpl() {
	if (pPlayer) {
	  --pPlayer->refCount;
	}
  }
  virtual bool IsPrepared() const { return true; }
  virtual void Play(float volume) {
	if (! pPlayer || !pPlayer->player) {
	  return;
	}
	(*pPlayer->playInterface)->SetPlayState(pPlayer->playInterface, SL_PLAYSTATE_STOPPED);
	(*pPlayer->bufferQueueInterface)->Enqueue(pPlayer->bufferQueueInterface, pPlayer->pSource->data.data(), pPlayer->pSource->data.size());
	(*pPlayer->volumeInterface)->SetVolumeLevel(pPlayer->volumeInterface, GetMillibelVolume(volume));
	(*pPlayer->playInterface)->SetPlayState(pPlayer->playInterface, SL_PLAYSTATE_PLAYING);
  }
private:
  BufferQueueAudioPlayer* pPlayer;
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
  std::array<BufferQueueAudioPlayer, 8>  sePlayerList;
  std::map<std::string, WaveSource> seList;
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

/** Create audio player object.
@sa DestroyMidiPlayer()
*/
bool CreateAudioPlayer(BufferQueueAudioPlayer& mp, SLEngineItf& eng, SLObjectItf& mix, const WaveSource& wave) {
  SLDataLocator_AndroidSimpleBufferQueue bqLoc = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
  SLDataSource fileSrc = { &bqLoc, const_cast<SLDataFormat_PCM*>(&wave.pcm) };

  SLDataLocator_OutputMix audOutLoc = { SL_DATALOCATOR_OUTPUTMIX, mix };
  SLDataSink audOutSnk = { &audOutLoc, nullptr };

  const SLboolean required[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };
  const SLInterfaceID iidArray[] = { SL_IID_PLAY, SL_IID_VOLUME, SL_IID_ANDROIDSIMPLEBUFFERQUEUE };

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
  result = (*mp.player)->GetInterface(mp.player, SL_IID_VOLUME, &mp.volumeInterface);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get SL_IID_VOLUME interface:0x%lx", result);
	return false;
  }
  result = (*mp.player)->GetInterface(mp.player, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &mp.bufferQueueInterface);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get SL_IID_ANDROIDSIMPLEBUFFERQUEUE interface:0x%lx", result);
	return false;
  }
  result = (*mp.playInterface)->GetDuration(mp.playInterface, &mp.dur);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to get duration:0x%lx", result);
	return false;
  }
  result = (*mp.bufferQueueInterface)->RegisterCallback(mp.bufferQueueInterface, BufferQueueCallback, &mp);
  if (result != SL_RESULT_SUCCESS) {
	LOGW("Failed to register callback:0x%lx", result);
	return false;
  }

  mp.pSource = &wave;

  LOGI("Create BufferQueueAudioPlayer");
  return true;
}

/** Destroy audio player object.

  If mp was destroyed already, this function does nothing.

  @param mp  the reference to the AudioPlayer that is destroyed.

  @sa AudioPlayer, CreateMidiPlayer()
*/
void DestroyAudioPlayer(AudioPlayer& mp) {
  if (mp.player) {
	(*mp.player)->Destroy(mp.player);
	mp.player = nullptr;
	LOGI("Destroy AudioPlayer");
  }
}

/** Destroy the buffer quene audio player object.

  If mp was destroyed already, this function does nothing.

  @param mp  the reference to the BufferQueueAudioPlayer that is destroyed.

  @sa BufferQueueAudioPlayer, CreateMidiPlayer()
*/
void DestroyAudioPlayer(BufferQueueAudioPlayer& mp) {
  if (mp.player) {
	(*mp.player)->Destroy(mp.player);
	mp.player = nullptr;
	mp.pSource = nullptr;
	LOGI("Destroy BufferQueueAudioPlayer");
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
  for (auto& e : sePlayerList) {
	DestroyAudioPlayer(e);
  }
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
  seList.clear();
}

struct RiffHeader {
  uint32_t riff; ///< "RIFF"
  uint32_t size;
  uint32_t type; ///< "WAVE"
};

struct WaveFormatChunk {
  uint32_t id; ///< "fmt "
  uint32_t size; /// 16

  uint16_t format;
  uint16_t channels;
  uint32_t samplePerSec;
  uint32_t avgBytePerSec;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
};

struct DataFormatChunk {
  uint32_t id; ///< "data"
  uint32_t size;
};

struct WaveFileHeader {
  RiffHeader riff;
  WaveFormatChunk wave;
  DataFormatChunk data;
};

constexpr uint32_t makeFourCC(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  return a + (b << 8) + (c << 16) + (d << 24);
}

/** Load the wave file used for SE from the Asset Manager.

  @param id       the identify of SE. it send to PlaySE(), PlaySE().
  @param filename the wave filename.

  @retval true  Loading succeeded and the wave data was stored on seList.
  @retval false Loading failed.
*/
bool AudioImpl::LoadSE(const char* id, const char* filename) {
  if (seList.find(id) != seList.end()) {
	return true;
  }
  FilePtr file = FileSystem::Open(filename);
  if (!file) {
	return false;
  }
  WaveSource ws;
  WaveFileHeader wf;
  file->Read(&wf, sizeof(wf));
  if (wf.riff.riff != makeFourCC('R', 'I', 'F', 'F') || wf.riff.type != makeFourCC('W', 'A', 'V', 'E')) {
	return false;
  }
  ws.data.resize(wf.data.size);
  file->Read(&ws.data[0], wf.data.size);

  ws.pcm.formatType = SL_DATAFORMAT_PCM;
  ws.pcm.numChannels = wf.wave.channels;
  ws.pcm.samplesPerSec = wf.wave.samplePerSec * 1000U;
  ws.pcm.bitsPerSample = wf.wave.bitsPerSample;
  ws.pcm.containerSize = wf.wave.bitsPerSample;
  ws.pcm.channelMask = wf.wave.channels == 1 ? SL_SPEAKER_FRONT_CENTER : (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT);
  ws.pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;

  ws.id = id;

  seList.emplace(id, std::move(ws));

  return true;
}

AudioCuePtr AudioImpl::PrepareSE(const char* id)  {
  BufferQueueAudioPlayer* pPlayer = nullptr;
  for (auto& e : sePlayerList) {
	if (!e.pSource) {
	  pPlayer = &e;
	}
  }
  if (!pPlayer) {
	return nullptr;
  }
  auto se = seList.find(id);
  if (se == seList.end()) {
	return nullptr;
  }
  if (!CreateAudioPlayer(*pPlayer, engineInterface, mixObject, se->second)) {
	return nullptr;
  }
  return AudioCuePtr(new AudioCueImpl(pPlayer));
}

void AudioImpl::PlaySE(const char* id, float volume) {
  if (auto cue = PrepareSE(id)) {
	cue->Play(volume);
  }
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
	const SLmillibel millibell = GetMillibelVolume(volume);
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
  for (auto& e : sePlayerList) {
	if (!e.player || e.refCount > 0) {
	  continue;
	}
	SLuint32 state;
	(*e.playInterface)->GetPlayState(e.playInterface, &state);
	if (state != SL_PLAYSTATE_PLAYING) {
	  DestroyAudioPlayer(e);
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
