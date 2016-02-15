#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED
#include <memory>
#include <vector>
#include <stdint.h>
#include <boost/optional.hpp>
#ifdef __ANDROID__
struct AAssetManager;
#endif

namespace Mai {
  class File {
  public:
	virtual ~File() {}
	virtual size_t Size() const = 0;
	virtual size_t Position() const = 0;
	virtual void Seek(size_t) = 0;
	virtual int Read(void*, size_t) = 0;
	virtual void Close() = 0;
  };
  typedef std::shared_ptr<File> FilePtr;

  namespace FileSystem {
#ifdef __ANDROID__
	void Initialize(AAssetManager*);
	struct FileDescriptor {
		int fd;
		off_t start;
		off_t length;
	};
	boost::optional<FileDescriptor> GetFileDescriptor(const char*);
#else
	void Initialize();
#endif
	FilePtr Open(const char*);

	typedef std::vector<uint8_t> RawBufferType;
	boost::optional<RawBufferType> LoadFile(const char*);
  }
}


#endif // FILE_H_INCLUDED
