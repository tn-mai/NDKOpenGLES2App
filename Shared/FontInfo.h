/**
  @file FontInfo.h
*/
#ifndef MAI_FONTINFO_H_INCLUDED
#define MAI_FONTINFO_H_INCLUDED
#include "Vector.h"

namespace Mai {

/** Text font size information.
*/
struct FontInfo {
  Position2S  leftTop; ///< left and top in texture(0.0-1.0 = 0-65535).
  Position2S  rightBottom; ///< right and bottom in texture(0.0-1.0 = 0-65535).

  /// Get font width by texel.
  GLushort GetWidth() const;

  /// Get font height by texel.
  GLushort GetHeight() const;
};

struct FontVertex {
  Position2F position;
  Position2S texCoord;
  Color4B  color;
};

enum EmojiFontId {
  EMOJIFONTID_EGG = 0x80,
};
static const size_t EMOJIFONTID_Max = 1;

enum SpecialFontId {
  SPECIALFONTID_READY,
  SPECIALFONTID_3,
  SPECIALFONTID_2,
  SPECIALFONTID_1,
  SPECIALFONTID_GO,
  SPECIALFONTID_LOOKSYUMMY,
  SPECIALFONTID_ITSEDIBLE,
  SPECIALFONTID_TOUCHME,
  SPECIALFONTID_WRONG,
  SPECIALFONTID_THATSNOTTHEPOINT,
  SPECIALFONTID_Max,
};

const FontInfo& GetAsciiFontInfo(int);
const FontInfo& GetSpecialFontInfo(SpecialFontId);

} // namespace Mai

#endif // MAI_FONTINFO_H_INCLUDED
