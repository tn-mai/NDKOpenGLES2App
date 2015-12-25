#ifndef ETC1_HEADER_INCLUDED
#define ETC1_HEADER_INCLUDED
#include <memory>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct android_app;

namespace Texture {
	class ITexture {
	public:
		ITexture() {}
		virtual ~ITexture() {}

		virtual GLuint TextureId() const = 0;
		virtual GLenum InternalFormat() const = 0;
		virtual GLenum Target() const = 0;
		virtual GLsizei Width() const = 0;
		virtual GLsizei Height() const = 0;
	};

	typedef std::shared_ptr<ITexture> TexturePtr;

	TexturePtr CreateEmpty2D(int, int);
	TexturePtr CreateDummy2D();
	TexturePtr CreateDummyCubeMap();
	TexturePtr LoadKTX(android_app*, const char*);
}

#endif // ETC1_HEADER_INCLUDED