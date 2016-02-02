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
  FONTINFO(0, 0, 20, 0), // (space)
  FONTINFO(0, 0, 20, 52), // !
  FONTINFO(20, 0, 50, 52), // " 
  FONTINFO(50, 0, 72, 52), // #
  FONTINFO(72, 0, 92, 52), // $
  FONTINFO(92, 0, 130, 52), // %
  FONTINFO(130, 0, 160, 52), // &
  FONTINFO(160, 0, 174, 52), // '
  FONTINFO(174, 0, 188, 52), // (
  FONTINFO(188, 0, 202, 52), // )
  FONTINFO(202, 0, 224, 52), // *
  FONTINFO(225, 0, 248, 52), // +
  FONTINFO(248, 0, 264, 52), // ,
  FONTINFO(264, 0, 297, 52), // -
  FONTINFO(297, 0, 314, 52), // .
  FONTINFO(314, 0, 344, 52), // /

  FONTINFO(  0, 55, 35,  101), // 0
  FONTINFO( 35, 55, 54,  101), // 1
  FONTINFO( 54, 55, 80,  101), // 2 
  FONTINFO( 80, 55, 110, 101), // 3
  FONTINFO(110, 55, 143, 101), // 4
  FONTINFO(143, 55, 171, 101), // 5
  FONTINFO(171, 55, 199, 101), // 6
  FONTINFO(199, 55, 229, 101), // 7
  FONTINFO(229, 55, 257, 101), // 8
  FONTINFO(257, 55, 286, 101), // 9
  FONTINFO(286, 55, 303, 101), // ;
  FONTINFO(303, 55, 321, 101), // ;
  FONTINFO(321, 55, 355, 101), // <
  FONTINFO(355, 55, 391, 101), // =
  FONTINFO(391, 55, 424, 101), // >
  FONTINFO(424, 55, 452, 101), // ?

  FONTINFO(  0, 101,  26, 142), // @
  FONTINFO( 26, 101,  60, 142), // A
  FONTINFO( 60, 101,  96, 142), // B
  FONTINFO( 96, 101, 130, 142), // C
  FONTINFO(130, 101, 162, 142), // D
  FONTINFO(162, 101, 190, 142), // E
  FONTINFO(190, 101, 218, 142), // F
  FONTINFO(218, 101, 254, 142), // G
  FONTINFO(254, 101, 293, 142), // H
  FONTINFO(293, 101, 312, 142), // I
  FONTINFO(312, 101, 342, 142), // J
  FONTINFO(342, 101, 379, 142), // K
  FONTINFO(379, 101, 405, 142), // L
  FONTINFO(405, 101, 444, 142), // M
  FONTINFO(444, 101, 472, 142), // N
  FONTINFO(472, 101, 507, 142), // O

  FONTINFO(  0, 150,  36,  197), // P
  FONTINFO( 36, 150,  75,  197), // Q
  FONTINFO( 75, 150, 111,  197), // R
  FONTINFO(111, 150, 138,  197), // S
  FONTINFO(138, 150, 170,  197), // T
  FONTINFO(170, 150, 201, 197), // U
  FONTINFO(201, 150, 230, 197), // V
  FONTINFO(230, 150, 279, 197), // W
  FONTINFO(279, 150, 314, 197), // X
  FONTINFO(314, 150, 349, 197), // Y
  FONTINFO(349, 150, 377, 197), // Z
  FONTINFO(377, 150, 395, 197), // [
  FONTINFO(395, 150, 418, 197), // '\'
  FONTINFO(418, 150, 433, 197), // ]
  FONTINFO(433, 150, 468, 197), // ^
  FONTINFO(468, 150, 503, 197), // _
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
  if (code >= 'a' && code <= 'z') {
	code -= 'a' - 'A';
  } else if (code < ' ' || code > '_') {
    return dummyFontInfo;
  }
  return asciiFontInfo[code - ' '];
}

const FontInfo& GetSpecialFontInfo(SpecialFontId id) {
  if (id < 0 || id > SPECIALFONTID_Max) {
    return dummyFontInfo;
  }
  return specialFontInfo[id];
}

} // namespace Mai
