#include "File.h"
#ifdef __ANDROID__
#include "android_native_app_glue.h"
#endif // __ANDROID__
#include <cstdio>

namespace Mai {
#ifdef __ANDROID__
  class AndroidFile : public File {
  public:
	AndroidFile(AAsset* p) : pAsset(p) {}
	virtual ~AndroidFile() { Close(); }
	virtual size_t Size() const {
	  return pAsset ? AAsset_getLength(pAsset) : 0;
	}
	virtual size_t Position() const {
	  return pAsset ? AAsset_seek(pAsset, 0, SEEK_CUR) : 0;
	}
	virtual void Seek(size_t n) {
	  if (pAsset) {
		AAsset_seek(pAsset, n, SEEK_SET);
	  }
	}
	virtual int Read(void* p, size_t n) {
	  return pAsset ? AAsset_read(pAsset, p, n) : 0;
	}
	virtual void Close() {
	  if (pAsset) {
		AAsset_close(pAsset);
		pAsset = nullptr;
	  }
	}
  private:
	AAsset* pAsset;
  };
#else
  class Win32File : public File {
  public:
	Win32File(FILE* p) : fp(p) {
	  std::fseek(fp, 0L, SEEK_END);
	  size = std::ftell(fp);
	  std::rewind(fp);
	}
	virtual ~Win32File() { Close(); }
	virtual size_t Size() const {
	  return size;
	}
	virtual size_t Position() const {
	  return fp ? std::ftell(fp) : 0;
	}
	virtual void Seek(size_t n) {
	  if (fp) {
		std::fseek(fp, n, SEEK_SET);
	  }
	}
	virtual int Read(void* p, size_t n) {
	  return fp ? std::fread(p, n, 1, fp) : 0;
	}
	virtual void Close() {
	  if (fp) {
		std::fclose(fp);
		fp = nullptr;
		size = 0;
	  }
	}
  private:
	FILE* fp;
	size_t size;
  };
#endif // __ANDROID__

  namespace FileSystem {
#ifdef __ANDROID__
	namespace {
	  AAssetManager* pAssetManager;
	} // unnamed namespace

	void Initialize(AAssetManager* p) {
	  pAssetManager = p;
	}

	FilePtr Open(const char* filepath) {
	  AAsset* pAsset = AAssetManager_open(pAssetManager, filepath, 0);
	  if (pAsset) {
		return std::make_shared<AndroidFile>(pAsset);
	  }
	  return nullptr;
    }
#else
	void Initialize() {
	}

	FilePtr Open(const char* filepath) {
	  FILE* fp;
	  std::string path("../OpenGLESApp2/OpenGLESApp2.Android.Packaging/assets/");
	  path += filepath;
	  fopen_s(&fp, path.c_str(), "rb");
	  if (fp) {
		return std::make_shared<Win32File>(fp);
	  }
	  return nullptr;
	}
#endif

	boost::optional<RawBufferType> LoadFile(const char* filepath) {
	  RawBufferType buf;
	  auto file = Open(filepath);
	  if (!file) {
		return boost::none;
	  }
	  const size_t size = file->Size();
	  buf.resize(size);
	  const int result = file->Read(&buf[0], size);
	  file->Close();
	  if (result < 0) {
		return boost::none;
	  }
	  return buf;
	}
  }
}
