/**
* @file Lnadscape.h
*/
#ifndef SSU_SHARED_SCENES_LANDSCAPE_H_INCLUDED
#define SSU_SHARED_SCENES_LANDSCAPE_H_INCLUDED
#include "../../OpenGLESApp2/OpenGLESApp2.Android.NativeActivity/Renderer.h"
#include <vector>

namespace SunnySideUp {
namespace Landscape {

  typedef std::vector<Mai::ObjectPtr> ObjectList;

  ObjectList GetCoast(Mai::Renderer&, const Mai::Vector3F&, float, Mai::ShadowCapability = Mai::ShadowCapability::Disable);

} // namespace Landscape
} // namespace SunnySideUp

#endif // SSU_SHARED_SCENES_LANDSCAPE_H_INCLUDED