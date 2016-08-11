/**
  @file FontInfo.cpp
*/
#include "FontInfo.h"
#include "../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"

#define WIDTH 512
#define HEIGHT 1024
#define RANGE 65535
#define FONTINFO(left, top, right, bottom) { { (left * RANGE) / WIDTH, ((HEIGHT - top) * RANGE) / HEIGHT }, { (right * RANGE) / WIDTH, ((HEIGHT - bottom) * RANGE) / HEIGHT } }

namespace Mai {

GLushort FontInfo::GetWidth() const {
  return static_cast<GLushort>((rightBottom.x - leftTop.x) * WIDTH / RANGE);
}

GLushort FontInfo::GetHeight() const {
  return static_cast<GLushort>((leftTop.y - rightBottom.y) * HEIGHT / RANGE);
}

const FontInfo dummyFontInfo = FONTINFO(0, 0, 0, 0);

const FontInfo asciiFontInfo[] = {
  FONTINFO(488, 4, 510,  50), // (space)
  FONTINFO(2,   4,  22,  50), // !
  FONTINFO(26,  4,  50,  50), // " 
  FONTINFO(58,  4,  80,  50), // #
  FONTINFO(84,  4, 104,  50), // $
  FONTINFO(108, 4, 144,  50), // %
  FONTINFO(150, 4, 174,  50), // &
  FONTINFO(178, 4, 194,  50), // '
  FONTINFO(196, 4, 212,  50), // (
  FONTINFO(216, 4, 232,  50), // )
  FONTINFO(236, 4, 256,  50), // *
  FONTINFO(260, 4, 284,  50), // +
  FONTINFO(288, 4, 304,  50), // ,
  FONTINFO(308, 4, 338,  50), // -
  FONTINFO(342, 4, 360,  50), // .
  FONTINFO(362, 4, 390,  50), // /

  FONTINFO(  2, 96, 36,  138), // 0
  FONTINFO( 38, 96, 60,  138), // 1
  FONTINFO( 62, 96, 90,  138), // 2 
  FONTINFO( 91, 96, 120, 138), // 3
  FONTINFO(122, 96, 154, 138), // 4
  FONTINFO(158, 96, 185, 138), // 5
  FONTINFO(188, 96, 216, 138), // 6
  FONTINFO(219, 96, 248, 138), // 7
  FONTINFO(251, 96, 278, 138), // 8
  FONTINFO(282, 96, 310, 138), // 9
  FONTINFO( 40, 52,  58,  98), // ;
  FONTINFO( 62, 52,  80,  98), // ;
  FONTINFO( 86, 52, 114,  98), // <
  FONTINFO(120, 52, 153,  98), // =
  FONTINFO(158, 52, 188,  98), // >
  FONTINFO(192, 52, 219,  98), // ?

  FONTINFO(220,  52, 248,  98), // @
  FONTINFO(312,  98, 346, 140), // A
  FONTINFO(348,  98, 384, 140), // B
  FONTINFO(387,  98, 420, 140), // C
  FONTINFO(422,  98, 453, 140), // D
  FONTINFO(456,  98, 484, 140), // E
  FONTINFO(  2, 140,  30, 182), // F
  FONTINFO( 34, 140,  68, 182), // G
  FONTINFO( 70, 140, 108, 182), // H
  FONTINFO(110, 140, 132, 182), // I
  FONTINFO(134, 140, 162, 182), // J
  FONTINFO(166, 140, 200, 182), // K
  FONTINFO(204, 140, 230, 182), // L
  FONTINFO(234, 140, 270, 182), // M
  FONTINFO(274, 140, 300, 182), // N
  FONTINFO(302, 140, 336, 182), // O

  FONTINFO(340, 140, 374, 182), // P
  FONTINFO(378, 140, 413, 182), // Q
  FONTINFO(416, 140, 452, 182), // R
  FONTINFO(455, 140, 480, 182), // S
  FONTINFO(  2, 182,  34, 224), // T
  FONTINFO( 36, 182,  66, 224), // U
  FONTINFO( 70, 182,  98, 224), // V
  FONTINFO(100, 182, 146, 224), // W
  FONTINFO(150, 182, 182, 224), // X
  FONTINFO(186, 182, 218, 224), // Y
  FONTINFO(222, 182, 249, 224), // Z
  FONTINFO(324,  52, 341,  98), // [
  FONTINFO(346,  52, 366,  98), // '\'
  FONTINFO(341,  52, 324,  98), // ]
  FONTINFO(252,  52, 282,  98), // ^
  FONTINFO(287,  52, 319,  98), // _

  FONTINFO(393,   4, 409,  50), // `
  FONTINFO(312,  98, 346, 140), // A
  FONTINFO(348,  98, 384, 140), // B
  FONTINFO(387,  98, 420, 140), // C
  FONTINFO(422,  98, 453, 140), // D
  FONTINFO(456,  98, 484, 140), // E
  FONTINFO(2, 140,  30, 182), // F
  FONTINFO(34, 140,  68, 182), // G
  FONTINFO(70, 140, 108, 182), // H
  FONTINFO(110, 140, 132, 182), // I
  FONTINFO(134, 140, 162, 182), // J
  FONTINFO(166, 140, 200, 182), // K
  FONTINFO(204, 140, 230, 182), // L
  FONTINFO(234, 140, 270, 182), // M
  FONTINFO(274, 140, 300, 182), // N
  FONTINFO(302, 140, 336, 182), // O

  FONTINFO(340, 140, 374, 182), // P
  FONTINFO(378, 140, 413, 182), // Q
  FONTINFO(416, 140, 452, 182), // R
  FONTINFO(455, 140, 480, 182), // S
  FONTINFO(2, 182,  34, 224), // T
  FONTINFO(36, 182,  66, 224), // U
  FONTINFO(70, 182,  98, 224), // V
  FONTINFO(100, 182, 146, 224), // W
  FONTINFO(150, 182, 182, 224), // X
  FONTINFO(186, 182, 218, 224), // Y
  FONTINFO(222, 182, 249, 224), // Z
  FONTINFO(412,   4, 437,  50), // {
  FONTINFO(440,   4, 456,  50), // |
  FONTINFO(461,   4, 487,  50), // }
  FONTINFO(  2,  52,  37,  98), // ~
  FONTINFO(  0,   4,   0,  50), // (del)

  FONTINFO(  2, 223,  36,  264), // Egg
};

const FontInfo specialFontInfo[] = {
  FONTINFO(  0, 200, 240, 257), // READY!
  FONTINFO(241, 200, 278, 257), // 3
  FONTINFO(280, 200, 313, 257), // 2
  FONTINFO(314, 200, 338, 257), // 1
  FONTINFO(339, 200, 457, 257), // GO!
  FONTINFO(  0, 258, 492, 315), // LOOKS YUMMY!
  FONTINFO(  0, 316, 434, 373), // IT'S EDIBLE...
  FONTINFO(  0, 375, 355, 432), // TOUCH ME!
  FONTINFO(  0, 433, 328, 490), // ...WRONG.
  FONTINFO(  0, 491, 381, 606), // THAT'S NOT THE POINT!
};

const FontInfo& GetAsciiFontInfo(int code) {
  // for the special characters(0x80-).
  // it will be used for emoji.
  if (code >= ' ' && code < 0x80 + EMOJIFONTID_Max) {
	return asciiFontInfo[code - ' '];
  }
  return dummyFontInfo;
}

const FontInfo& GetSpecialFontInfo(SpecialFontId id) {
  if (id < 0 || id > SPECIALFONTID_Max) {
    return dummyFontInfo;
  }
  return specialFontInfo[id];
}

} // namespace Mai
