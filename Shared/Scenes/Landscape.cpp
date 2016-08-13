/**
* @file Landscape.cpp
*/
#include "Landscape.h"
#include "Scene.h"
#include "../File.h"
#include <cmath>

namespace SunnySideUp {
namespace Landscape {

  using namespace Mai;

  /**
  * Create the landscape of coast.
  *
  * @param  renderer  The renderer that contains the model object.
  * @param  offset    The landscape is offseted by this value.
  * @param  scale     The landscape is scaled by this value.
  * @param  shadowCapability  Whether the landscape cast a shadow.
  *
  * @return the object list that make up the landscape of coast.
  */
  ObjectList GetCoast(Renderer& renderer, const Vector3F& offset, float scale, ShadowCapability shadowCapability) {

	renderer.LoadLandscape(LandscapeOfScene_Coast, renderer.GetTimeOfScene());

	const Vector3F scale3(scale, scale, scale);
	ObjectList objList;

	if (auto pBuf = FileSystem::LoadFile("Meshes/CoastTownSpace.msh")) {
	  const auto result = Mesh::ImportGeometry(*pBuf);
	  if (result.result == Mesh::Result::success && !result.geometryList.empty()) {
		const Mesh::Geometry& m = result.geometryList[0];
		static const char* const meshNameList[] = {
		  "CityBlock00",
		  "CityBlock01",
		  "CityBlock02",
		  "CityBlock03",
		  "CityBlock04",
		  "CityBlock05",
		  "CityBlock06",
		  "CityBlock07",
		};
		static const Color4B colorList[] = {
		  Color4B(200, 200, 200, 255),
		  Color4B(255, 200, 200, 255),
		  Color4B(200, 255, 200, 255),
		  Color4B(200, 200, 255, 255),
		  Color4B(255, 255, 200, 255),
		};
		int i = 0;
		for (const auto& e : m.vertexList) {
		  auto obj = renderer.CreateObject(
			meshNameList[i % (sizeof(meshNameList) / sizeof(meshNameList[0]))],
			Material(colorList[i % (sizeof(colorList) / sizeof(colorList[0]))], 0, 0),
			"default",
			ShadowCapability::Disable
		  );
		  obj->SetScale(Vector3F(4, 4, 4));
		  obj->SetTranslation(Vector3F(e.position.x * scale, e.position.y * scale, e.position.z * scale) + offset);
		  const float ry = std::asin(e.tangent.z / e.tangent.x);
		  obj->SetRotation(degreeToRadian<float>(0), ry, degreeToRadian<float>(0));
		  objList.push_back(obj);
		  ++i;
		}
	  }
	}
	{
	  auto obj = renderer.CreateObject("LandScape.Coast.Levee", Material(Color4B(200, 200, 200, 255), 0, 0), "default", shadowCapability);
	  obj->SetScale(scale3);
	  obj->SetTranslation(offset);
	  objList.push_back(obj);
	}
	{
	  auto obj = renderer.CreateObject("LandScape.Coast", Material(Color4B(200, 200, 200, 255), 0, 0), "sea", shadowCapability);
	  obj->SetScale(scale3);
	  obj->SetTranslation(offset);
	  objList.push_back(obj);
	}
	{
	  auto obj = renderer.CreateObject("LandScape.Coast.Flora", Material(Color4B(128, 128, 128, 255), 0, 0), "defaultWithAlpha", shadowCapability);
	  obj->SetScale(scale3);
	  obj->SetTranslation(offset);
	  objList.push_back(obj);
	}
	{
	  auto obj = renderer.CreateObject("LandScape.Coast.Ships", Material(Color4B(200, 200, 200, 255), 0, 0), "defaultWithAlpha", shadowCapability);
	  obj->SetScale(scale3);
	  obj->SetTranslation(offset);
	  objList.push_back(obj);
	}
	return objList;
  }

} // namespace Landscape
} // namespace SunnySideUp