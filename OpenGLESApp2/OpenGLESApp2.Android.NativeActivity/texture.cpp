#include "texture.h"
#include "../../Shared/File.h"
#ifdef __ANDROID__
#include "android_native_app_glue.h"
#include <android/log.h>
#endif // __ANDROID__
#include <vector>

#ifdef __ANDROID__
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "texture.cpp", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "texture.cpp", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf("texture.cpp" __VA_ARGS__))
#define LOGW(...) ((void)printf("texture.cpp" __VA_ARGS__))
#endif // __ANDROID__

namespace Texture {

	struct KTXHeader {
		uint8_t identifier[12];
		uint32_t endianness;
		uint32_t glType;
		uint32_t glTypeSize;
		uint32_t glFormat;
		uint32_t glInternalFormat;
		uint32_t glBaseInternalFormat;
		uint32_t pixelWidth;
		uint32_t pixelHeight;
		uint32_t pixelDepth;
		uint32_t numberOfArrayElements;
		uint32_t numberOfFaces;
		uint32_t numberOfMipmapLevels;
		uint32_t bytesOfKeyValueData;
	};

	enum Endian {
		Endian_Little,
		Endian_Big,
		Endian_Unknown,
	};

	Endian GetEndian(const KTXHeader& h) {
		const uint8_t* p = reinterpret_cast<const uint8_t*>(&h.endianness);
		if (p[0] == 0x01 && p[1] == 0x02 && p[2] == 0x03 && p[3] == 0x04) {
			return Endian_Little;
		} else if (p[0] == 0x04 && p[1] == 0x03 && p[2] == 0x02 && p[3] == 0x01) {
			return Endian_Big;
		}
		return Endian_Unknown;
	}

	uint32_t GetValue(const uint32_t* pBuf, Endian e) {
		const uint8_t* p = reinterpret_cast<const uint8_t*>(pBuf);
		if (e == Endian_Little) {
			return p[3] * 0x1000000 + p[2] * 0x10000 + p[1] * 0x100 + p[0];
		}
		return p[0] * 0x1000000 + p[1] * 0x10000 + p[2] * 0x100 + p[3];
	}

	bool IsKTXHeader(const KTXHeader& h) {
		static const uint8_t fileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};
		for (int i = 0; i < sizeof(fileIdentifier); ++i) {
			if (h.identifier[i] != fileIdentifier[i]) {
				return false;
			}
		}
		return true;
	}

	struct Texture : public ITexture {
		GLuint texId;
		GLenum internalFormat;
		GLenum target;
		uint16_t width;
		uint16_t height;

		Texture() : texId(0) {}
		virtual ~Texture() {
			if (texId) {
				glDeleteTextures(1, &texId);
			}
		}
		virtual GLuint TextureId() const { return texId; }
		virtual GLenum InternalFormat() const { return internalFormat; }
		virtual GLenum Target() const { return target; }
		virtual GLsizei Width() const { return width; }
		virtual GLsizei Height() const { return height; }
	};

	/** 空のテクスチャを作成する.
	*/
	TexturePtr CreateEmpty2D(int w, int h) {
		TexturePtr p = std::make_shared<Texture>();
		Texture& tex = static_cast<Texture&>(*p);
		tex.internalFormat = GL_RGBA;
		tex.width = w;
		tex.height = h;
		tex.target = GL_TEXTURE_2D;

		glGenTextures(1, &tex.texId);
		glBindTexture(tex.Target(), tex.texId);
		glTexImage2D(GL_TEXTURE_2D, 0, tex.InternalFormat(), tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(tex.Target(), 0);
		return p;
	}

	/** ダミー2Dテクスチャを作成する.
	*/
	TexturePtr CreateDummy2D() {
		TexturePtr p = std::make_shared<Texture>();
		Texture& tex = static_cast<Texture&>(*p);
		tex.internalFormat = GL_RGBA;
		tex.width = 1;
		tex.height = 1;
		tex.target = GL_TEXTURE_2D;
		glGenTextures(1, &tex.texId);
		glBindTexture(tex.Target(), tex.texId);
		static const uint8_t pixels[] = { 0xF0, 0x40, 0x20, 0xff };
		glTexImage2D(GL_TEXTURE_2D, 0, tex.InternalFormat(), 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		const GLenum result = glGetError();
		if (result != GL_NO_ERROR) {
		  LOGW("glTexImage2D error 0x%04x", result);
		}
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(tex.Target(), 0);
		return p;
	}

	/** ダミーキューブマップを作成する.
	*/
	TexturePtr CreateDummyCubeMap() {
		TexturePtr p = std::make_shared<Texture>();
		Texture& tex = static_cast<Texture&>(*p);
		tex.internalFormat = GL_RGBA;
		tex.width = 1;
		tex.height = 1;
		tex.target = GL_TEXTURE_CUBE_MAP;
		glGenTextures(1, &tex.texId);
		glBindTexture(tex.Target(), tex.texId);
		/*
		     +--+
			 |-Y|
		  +--+--+--+--+
		  |+X|-Z|-X|+Z|
		  +--+--+--+--+
		     |+Y|
			 +--+
		*/
		static const uint8_t pixels[][4] = {
			{ 0xff, 0x00, 0x00, 0xff }, // GL_TEXTURE_CUBE_MAP_POSITIVE_X
			{ 0x00, 0xff, 0x00, 0xff }, // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
			{ 0x00, 0x00, 0x00, 0xff }, // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
			{ 0x00, 0x00, 0xff, 0xff }, // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
			{ 0xff, 0xff, 0x00, 0xff }, // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
			{ 0xff, 0x00, 0xff, 0xff }, // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		};
		for (int faceIndex = 0; faceIndex < 6; ++faceIndex) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, 0, tex.InternalFormat(), 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels[faceIndex]);
		}
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(tex.Target(), 0);
		return p;
	}

	/** KTXファイルを読み込む.
	*/
	TexturePtr LoadKTX(const char* filename) {
		auto file = Mai::FileSystem::Open(filename);
		if (!file) {
			LOGW("cannot open:'%s'", filename);
			return nullptr;
		}

		const size_t size = file->Size();
		if (size <= sizeof(KTXHeader)) {
			LOGW("illegal size:'%s'", filename);
			return nullptr;
		}
		KTXHeader header;
		int result = file->Read(&header, sizeof(KTXHeader));
		if (result < 0 || !IsKTXHeader(header)) {
			LOGW("illegal header:'%s'", filename);
			return nullptr;
		}
		TexturePtr p = std::make_shared<Texture>();
		Texture& tex = static_cast<Texture&>(*p);
		const Endian endianness = GetEndian(header);
		const GLenum type = GetValue(&header.glType, endianness);
		const GLenum format = GetValue(type ? &header.glFormat : &header.glBaseInternalFormat, endianness);
		const int faceCount = GetValue(&header.numberOfFaces, endianness);
		const int mipCount = GetValue(&header.numberOfMipmapLevels, endianness);
		tex.internalFormat = GetValue(type ? &header.glBaseInternalFormat : &header.glInternalFormat, endianness);
		tex.width = GetValue(&header.pixelWidth, endianness);
		tex.height = GetValue(&header.pixelHeight, endianness);
		tex.target = faceCount == 6 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

		const uint32_t off = GetValue(&header.bytesOfKeyValueData, endianness);
		file->Seek(file->Position() + off);

		glGenTextures(1, &tex.texId);
		glBindTexture(tex.Target(), tex.texId);
		std::vector<uint8_t> data;
		GLsizei curWidth = tex.Width();
		GLsizei curHeight = tex.Height();
		for (int mipLevel = 0; mipLevel < (mipCount ? mipCount : 1); ++mipLevel) {
			uint32_t imageSize;
			result = file->Read(&imageSize, 4);
			if (result < 0) {
				LOGW("can't read(miplevel=%d):'%s'", mipLevel, filename);
				return nullptr;
			}
			imageSize = GetValue(&imageSize, endianness);
			const uint32_t imageSizeWithPadding = (imageSize + 3) & ~3;
			data.resize(imageSizeWithPadding * faceCount);
			result = file->Read(&data[0], data.size());
			if (result < 0) {
				LOGW("can't read(miplevel=%d):'%s'", mipLevel, filename);
				return nullptr;
			}
			const uint8_t* pImage = reinterpret_cast<const uint8_t*>(&data[0]);
			const GLenum target = tex.Target() == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : GL_TEXTURE_2D;
			for (int faceIndex = 0; faceIndex < faceCount; ++faceIndex) {
				if (type == 0) {
					glCompressedTexImage2D(target + faceIndex, mipLevel, tex.InternalFormat(), curWidth, curHeight, 0, imageSize, pImage);
				} else {
					glTexImage2D(target + faceIndex, mipLevel, tex.InternalFormat(), curWidth, curHeight, 0, format, type, pImage);
				}
				const GLenum result = glGetError();
				switch(result) {
				case GL_NO_ERROR:
				  break;
				case GL_INVALID_OPERATION:
				  LOGW("glCompressed/TexImage2D return GL_INVALID_OPERATION");
				  break;
				default:
				  LOGW("glCompressed/TexImage2D return error code 0x%04x", result);
				  break;
				}
				pImage += imageSizeWithPadding;
			}
			curWidth = std::max(1, curWidth / 2);
			curHeight = std::max(1, curHeight / 2);
		}
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, mipCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(tex.Target(), 0);
		LOGI("Load %s(ID:%x).", filename, tex.texId);
		return p;
	}
}