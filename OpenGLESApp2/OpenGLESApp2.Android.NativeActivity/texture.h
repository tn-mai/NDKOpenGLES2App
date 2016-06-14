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

	TexturePtr CreateEmpty2D(int w, int h, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR);
	TexturePtr CreateDummy2D();
	TexturePtr CreateDummyNormal();
	TexturePtr CreateDummyCubeMap();
	TexturePtr LoadKTX(const char*, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR);
}

#endif // ETC1_HEADER_INCLUDED