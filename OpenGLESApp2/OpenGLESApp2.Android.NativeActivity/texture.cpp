#include "texture.h"
#include "../../Shared/File.h"
#ifdef __ANDROID__
#include "android_native_app_glue.h"
#include <android/log.h>
#endif // __ANDROID__
#include <vector>
#include <algorithm>

#ifdef __ANDROID__
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "texture.cpp", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "texture.cpp", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf("texture.cpp" __VA_ARGS__), (void)printf("\n"))
#define LOGW(...) ((void)printf("texture.cpp" __VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

namespace Texture {

  uint64_t totalByteSize = 0;

  /** Get the corrected texture filter mode.

    @param mipCount  The number of mipmap level.
	@param filter    The required filter mode.

	@return The filter mode corrected by mipCount.
  */
  GLint CorrectFilter(int mipCount, GLint filter) {
	if (mipCount > 1) {
	  switch (filter) {
	  case GL_NEAREST:
		return GL_NEAREST_MIPMAP_NEAREST;
	  case GL_LINEAR:
	  default:
		return GL_LINEAR_MIPMAP_LINEAR;
	  case GL_NEAREST_MIPMAP_NEAREST:
	  case GL_LINEAR_MIPMAP_NEAREST:
	  case GL_NEAREST_MIPMAP_LINEAR:
	  case GL_LINEAR_MIPMAP_LINEAR:
		return filter;
	  }
	}
	return filter == GL_NEAREST ? GL_NEAREST : GL_LINEAR;
  }

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
		uint32_t byteSize;

		Texture() : texId(0) {}
		virtual ~Texture() {
			if (texId) {
				glDeleteTextures(1, &texId);
				if (totalByteSize < byteSize) {
					LOGW("Total texture size mismatch: total/current(0x%llx/0x%x)", totalByteSize, byteSize);
					totalByteSize = 0;
				} else {
					totalByteSize -= byteSize;
				}
			}
		}
		virtual GLuint TextureId() const { return texId; }
		virtual GLenum InternalFormat() const { return internalFormat; }
		virtual GLenum Target() const { return target; }
		virtual GLsizei Width() const { return width; }
		virtual GLsizei Height() const { return height; }
	};

	bool Color1555To888(uint16_t color1555, uint32_t* pColor888) {
	  const uint32_t r = ((color1555 & 0x00007C00) >> 7) | ((color1555 & 0x00007000) >> 12);
	  const uint32_t g = ((color1555 & 0x000003E0) >> 2) | ((color1555 & 0x00000380) >> 7);
	  const uint32_t b = ((color1555 & 0x0000001F) << 3) | ((color1555 & 0x0000001C) >> 2);
	  *pColor888 = (b << 16) | (g << 8) | r;

	  return (color1555 & 0x8000) != 0;
	}

	void Color565To888(uint16_t color565, uint32_t* pColor888) {
	  const uint32_t r = ((color565 & 0x0000F800) >> 8) | ((color565 & 0x0000E000) >> 13);
	  const uint32_t g = ((color565 & 0x000007E0) >> 3) | ((color565 & 0x00000600) >> 9);
	  const uint32_t b = ((color565 & 0x0000001F) << 3) | ((color565 & 0x0000001C) >> 2);
	  *pColor888 = (b << 16) | (g << 8) | r;
	}

	template<uint32_t RightShift, uint32_t Mask = 0xff, typename R = uint32_t>
	R Elem(uint32_t rgba) { return static_cast<R>((rgba >> RightShift) & Mask); }

	void GetColors(uint16_t color0, uint16_t color1, uint32_t* col) {
	  static int      iColorLow = 0;
	  static int      iColorMedLow = 1;
	  static int      iColorMedHigh = 2;
	  static int      iColorHigh = 3;

	  const bool hasBlackTrick = Color1555To888(color0, &col[iColorLow]);
	  Color565To888(color1, &col[iColorHigh]);

	  if (hasBlackTrick) {
		col[iColorMedHigh] = col[iColorLow];

		col[iColorMedLow] = static_cast<uint32_t>(std::max(Elem<16, 0xff, int32_t>(col[iColorMedHigh]) - Elem<18, 0x3f, int32_t>(col[iColorHigh]), 0)) << 16;
		col[iColorMedLow] |= static_cast<uint32_t>(std::max(Elem<8, 0xff, int32_t>(col[iColorMedHigh]) - Elem<10, 0x3f, int32_t>(col[iColorHigh]), 0)) < 8;
		col[iColorMedLow] |= static_cast<uint32_t>(std::max(Elem<0, 0xff, int32_t>(col[iColorMedHigh]) - Elem<2, 0x3f, int32_t>(col[iColorHigh]), 0));

		col[iColorLow] = 0;
	  } else {
		col[iColorMedHigh] = (Elem<16>(col[iColorHigh]) * 5 + Elem<16>(col[iColorLow]) * 3) >> 3 << 16;
		col[iColorMedHigh] |= (Elem<8>(col[iColorHigh]) * 5 + Elem<8>(col[iColorLow]) * 3) >> 3 << 8;
		col[iColorMedHigh] |= (Elem<0>(col[iColorHigh]) * 5 + Elem<0>(col[iColorLow]) * 3) >> 3;

		col[iColorMedLow] = (Elem<16>(col[iColorHigh]) * 3 + Elem<16>(col[iColorLow]) * 5) >> 3 << 16;
		col[iColorMedLow] |= (Elem<8>(col[iColorHigh]) * 3 + Elem<8>(col[iColorLow]) * 5) >> 3 << 8;
		col[iColorMedLow] |= (Elem<0>(col[iColorHigh]) * 3 + Elem<0>(col[iColorLow]) * 5) >> 3;
	  }
	}

	struct ColorBlock {
	  uint16_t color0;
	  uint16_t color1;
	  uint8_t pixels[4];

	  void Get(uint32_t* rgba) const {
		uint32_t col[4];
		GetColors(color0, color1, col);
		for (int y = 0; y < 4; ++y) {
		  for (int x = 0; x < 4; ++x) {
			const uint32_t bits = (pixels[y] & (0x03 << (x * 2))) >> (x * 2);
			*(rgba++) = col[bits];
		  }
		}
	  }
	};

	struct ExplicitAlphaBlock {
	  uint8_t pixels[8];

	  void Get(uint32_t* rgba) const {
		for (int y = 0; y < 4; ++y) {
		  for (int x = 0; x < 4; ++x) {
			const uint32_t a = pixels[y * 2 + ((x / 2) & 1)] & (0x0f << ((x & 1) * 4));
			*(rgba++) = a | (a << 4);
		  }
		}
	  }
	};

	struct InterporatedAlphaBlock {
	  uint8_t alpha0;
	  uint8_t alpha1;
	  uint8_t pixels[6];

	  void GetCompressedAlphaRamp(uint8_t* result) const {
		result[0] = alpha0;
		result[1] = alpha1;

		if (alpha0 > alpha1) {
		  // 8-alpha block:  derive the other six alphas.
		  // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		  result[2] = static_cast<uint8_t>((6 * alpha0 + 1 * alpha1 + 3) / 7);    // bit code 010
		  result[3] = static_cast<uint8_t>((5 * alpha0 + 2 * alpha1 + 3) / 7);    // bit code 011
		  result[4] = static_cast<uint8_t>((4 * alpha0 + 3 * alpha1 + 3) / 7);    // bit code 100
		  result[5] = static_cast<uint8_t>((3 * alpha0 + 4 * alpha1 + 3) / 7);    // bit code 101
		  result[6] = static_cast<uint8_t>((2 * alpha0 + 5 * alpha1 + 3) / 7);    // bit code 110
		  result[7] = static_cast<uint8_t>((1 * alpha0 + 6 * alpha1 + 3) / 7);    // bit code 111
		} else {
		  // 6-alpha block.
		  // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		  result[2] = static_cast<uint8_t>((4 * alpha0 + 1 * alpha1 + 2) / 5);  // Bit code 010
		  result[3] = static_cast<uint8_t>((3 * alpha0 + 2 * alpha1 + 2) / 5);  // Bit code 011
		  result[4] = static_cast<uint8_t>((2 * alpha0 + 3 * alpha1 + 2) / 5);  // Bit code 100
		  result[5] = static_cast<uint8_t>((1 * alpha0 + 4 * alpha1 + 2) / 5);  // Bit code 101
		  result[6] = 0;                                      // Bit code 110
		  result[7] = 255;                                    // Bit code 111
		}
	  }

	  void Get(uint32_t* rgba) const {
		uint8_t ramp[8];
		GetCompressedAlphaRamp(ramp);
		for (int y = 0; y < 4; ++y) {
		  for (int x = 0; x < 4; ++x) {
			uint32_t index;
			const uint32_t i = (y * 4 + x);
			const uint32_t i0 = (i * 3) / 8;
			const uint32_t i1 = (i * 3 + 2) / 8;
			if (i0 == i1) {
			  index = (pixels[i0] >> ((i * 3) % 8)) & 0x7;
			} else {
			  index = ((pixels[i0] | (pixels[i1] << 8)) >> ((i * 3) % 8)) & 0x7;
			}
			*(rgba++) = ramp[index];
		  }
		}
	  }
	};

	struct ExplicitBlock {
	  ExplicitAlphaBlock alpha;
	  ColorBlock color;

	  void Get(uint32_t* rgba) const {
		uint32_t rgb[16];
		color.Get(rgb);
		uint32_t a[16];
		alpha.Get(a);
		for (int i = 0; i < 16; ++i) {
		  rgba[i] = rgb[i] | (a[i] << 24);
		}
	  }
	};

	struct InterporatedBlock {
	  InterporatedAlphaBlock alpha;
	  ColorBlock color;

	  void Get(uint32_t* rgba) const {
		uint32_t rgb[16];
		color.Get(rgb);
		uint32_t a[16];
		alpha.Get(a);
		for (int i = 0; i < 16; ++i) {
		  rgba[i] = rgb[i] | (a[i] << 24);
		}
	  }
	};

	/**
	* Convert ATITC to RGBBA8888.
	*
	* @sa https://github.com/GPUOpen-Tools/Compressonator
	*/
	std::vector<uint32_t> DecompressATITC(const uint8_t* pImage, uint32_t w, uint32_t h, GLenum format) {
	  std::vector<uint32_t> result;
	  result.resize(w * h);
	  const uint32_t xcount = w / 4;
	  switch (format) {
	  case GL_ATC_RGBA_EXPLICIT_ALPHA_AMD: {
		const ExplicitBlock* p = reinterpret_cast<const ExplicitBlock*>(pImage);
		for (uint32_t y = 0; y < h; y += 4) {
		  for (uint32_t x = 0; x < w; x += 4) {
			const ExplicitBlock& cur = p[(y / 4) * xcount + x / 4];
			uint32_t rgba[16];
			cur.Get(rgba);
			for (uint32_t yy = 0; yy < 4; ++yy) {
			  for (uint32_t xx = 0; xx < 4; ++xx) {
				result[(y + yy) * w + (x + xx)] = rgba[yy * 4 + xx];
			  }
			}
		  }
		}
		break;
	  }
	  case GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD: {
		const InterporatedBlock* p = reinterpret_cast<const InterporatedBlock*>(pImage);
		for (uint32_t y = 0; y < h; y += 4) {
		  for (uint32_t x = 0; x < w; x += 4) {
			const InterporatedBlock& cur = p[(y / 4) * xcount + x / 4];
			uint32_t rgba[16];
			cur.Get(rgba);
			for (uint32_t yy = 0; yy < 4; ++yy) {
			  for (uint32_t xx = 0; xx < 4; ++xx) {
				result[(y + yy) * w + (x + xx)] = rgba[yy * 4 + xx];
			  }
			}
		  }
		}
		break;
	  }
	  }
	  return result;
	}


	/** 空のテクスチャを作成する.
	*/
	TexturePtr CreateEmpty2D(int w, int h, GLint minFilter, GLint magFilter) {
		TexturePtr p = std::make_shared<Texture>();
		Texture& tex = static_cast<Texture&>(*p);
		tex.internalFormat = GL_RGBA;
		tex.width = w;
		tex.height = h;
		tex.target = GL_TEXTURE_2D;

		glGenTextures(1, &tex.texId);
		glBindTexture(tex.Target(), tex.texId);
		glTexImage2D(GL_TEXTURE_2D, 0, tex.InternalFormat(), tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		const GLenum result = glGetError();
		if (result != GL_NO_ERROR) {
		  LOGW("glTexImage2D error 0x%04x", result);
		}
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, CorrectFilter(1, minFilter));
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, CorrectFilter(1, magFilter));
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tex.byteSize = tex.width * tex.height * 4;
		totalByteSize += tex.byteSize;

		glBindTexture(tex.Target(), 0);
		LOGI("Load %s(ID:%x)(TOTAL:%lld).", "Empty2D", tex.texId, totalByteSize);
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
		tex.byteSize = tex.width * tex.height * 4;
		totalByteSize += tex.byteSize;

		glBindTexture(tex.Target(), 0);
		LOGI("Load %s(ID:%x)(TOTAL:%lld).", "Dummy2D", tex.texId, totalByteSize);
		return p;
	}

	/** ダミー法線テクスチャを作成する.
	*/
	TexturePtr CreateDummyNormal() {
	  TexturePtr p = std::make_shared<Texture>();
	  Texture& tex = static_cast<Texture&>(*p);
	  tex.internalFormat = GL_RGBA;
	  tex.width = 1;
	  tex.height = 1;
	  tex.target = GL_TEXTURE_2D;
	  glGenTextures(1, &tex.texId);
	  glBindTexture(tex.Target(), tex.texId);
	  static const uint8_t pixels[] = { 0x00, 0x80, 0x00, 0x80 };
	  glTexImage2D(GL_TEXTURE_2D, 0, tex.InternalFormat(), 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	  const GLenum result = glGetError();
	  if (result != GL_NO_ERROR) {
		LOGW("glTexImage2D error 0x%04x", result);
	  }
	  glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	  glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	  glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	  glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	  tex.byteSize = tex.width * tex.height * 4;
	  totalByteSize += tex.byteSize;

	  glBindTexture(tex.Target(), 0);
	  LOGI("Load %s(ID:%x)(TOTAL:%lld).", "DummyNormal", tex.texId, totalByteSize);
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
			const GLenum result = glGetError();
			if (result != GL_NO_ERROR) {
			  LOGW("glTexImage2D(face:%d) error 0x%04x", faceIndex, result);
			}
		}
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tex.byteSize = tex.width * tex.height * 4 * 6;
		totalByteSize += tex.byteSize;

		glBindTexture(tex.Target(), 0);
		LOGI("Load %s(ID:%x)(TOTAL:%lld).", "DummyCubeMap", tex.texId, totalByteSize);
		return p;
	}

	/** KTXファイルを読み込む.
	*/
	TexturePtr LoadKTX(const char* filename, bool decompressing, GLint minFilter, GLint magFilter) {
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
		tex.byteSize = 0;

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
				  if (decompressing) {
					std::vector<uint32_t> decompressedImage = DecompressATITC(pImage, curWidth, curHeight, tex.InternalFormat());
					glTexImage2D(target + faceIndex, mipLevel, GL_RGBA, curWidth, curHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, decompressedImage.data());
				  } else {
					glCompressedTexImage2D(target + faceIndex, mipLevel, tex.InternalFormat(), curWidth, curHeight, 0, imageSize, pImage);
				  }
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
			tex.byteSize += imageSizeWithPadding * faceCount;
			curWidth = std::max(1, curWidth / 2);
			curHeight = std::max(1, curHeight / 2);
		}
		glTexParameteri(tex.Target(), GL_TEXTURE_MIN_FILTER, CorrectFilter(mipCount, minFilter));
		glTexParameteri(tex.Target(), GL_TEXTURE_MAG_FILTER, CorrectFilter(1, magFilter));
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(tex.Target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		totalByteSize += tex.byteSize;

		glBindTexture(tex.Target(), 0);
		LOGI("Load %s(ID:%x)(TOTAL:%lld).", filename, tex.texId, totalByteSize);
		return p;
	}
}