#ifndef FBX_H_INCLUDED
#define FBX_H_INCLUDED

#include "../../Shared/FixedNum.h"
#include "../../Shared/Vector.h"
#include "../../Shared/Quaternion.h"
#include "../../Shared/Matrix.h"
#include "texture.h"
#include <vector>

namespace Mai {

/// Raw data input/output buffer type.
typedef std::vector<uint8_t> RawBuffer;

/// The count of texture coordinates at each vertex.
#define VERTEX_TEXTURE_COUNT_MAX (2)

/**
* The vertex format.
*/
struct Vertex {
  Position3F  position; ///< ���_���W. 12
  GLubyte     weight[4]; ///< ���_�u�����f�B���O�̏d��. 255 = 1.0�Ƃ��ėʎq�������l���i�[����. 4
  Vector3F    normal; ///< ���_�m�[�}��. 12
  GLubyte     boneID[4]; ///< ���_�u�����f�B���O�̃{�[��ID. 4
  Position2S  texCoord[VERTEX_TEXTURE_COUNT_MAX]; ///< �f�B�t���[�Y(with���^���l�X)�}�b�v���W, �m�[�}��(with���t�l�X)�}�b�v���W. 8
  Vector4F    tangent; ///< ���_�^���W�F���g. 16

  Vertex() {}
};

/**
* The material of mesh.
*/
struct Material {
  Color4B color;
  Fixed16 metallic;
  Fixed16 roughness;
  Material() {}
  constexpr Material(Color4B c, GLfloat m, GLfloat r) : color(c), metallic(Fixed16::From(m)), roughness(Fixed16::From(r)) {}
};

/**
* A couple of rotation and translation.
*
* The rotation is represented by Quaternion.
*/
struct RotTrans {
  Quaternion rot;
  Vector3F trans;
  RotTrans& operator*=(const RotTrans& rhs) {
	const Vector3F v = rot.Apply(rhs.trans);
	trans = v + trans;
	rot *= rhs.rot;
	rot.Normalize();
	return *this;
  }

  friend RotTrans operator*(const RotTrans& lhs, const RotTrans& rhs) { return RotTrans(lhs) *= rhs; }
  friend RotTrans Interporation(const RotTrans& lhs, const RotTrans& rhs, float t) {
	return{ Sleap(lhs.rot, rhs.rot, t), lhs.trans * (1.0f - t) + rhs.trans * t };
  }
  static const RotTrans& Unit() {
	static const RotTrans rt = { Quaternion::Unit(), Vector3F::Unit() };
	return rt;
  }
};
Matrix4x3 ToMatrix(const RotTrans& rt);

/**
* 1. �e�W���C���g�̌��݂̎p�����v�Z����(initial�ɃA�j���[�V�������ɂ��s����|����). ���̍s��̓W���C���g���[�J���ȕϊ��s��ƂȂ�.
* 2. �W���C���g���[�J���ȕϊ��s��ɁA���g���烋�[�g�W���C���g�܂ł̑S�Ă̐e�̎p���s����|���邱�ƂŁA���f�����[�J���ȕϊ��s��𓾂�.
* 3. ���f�����[�J���ȕϊ��s��ɋt�o�C���h�|�[�Y�s����|���A�ŏI�I�ȕϊ��s��𓾂�.
* ���̕ϊ��s��́A���f�����[�J�����W�n�ɂ��钸�_���W���C���g���[�J�����W�n�Ɉړ����A
* ���݂̎p���ɂ���ĕϊ����A�Ăу��f�����[�J�����W�n�ɖ߂��Ƃ���������s��.
*
* �t�o�C���h�|�[�Y�s�� = Inverse(�����p���s��)
*/
struct Joint {
  RotTrans invBindPose;
  RotTrans initialPose;
  int offChild; ///< 0: no child.
  int offSibling; ///< 0: no sibling.
};
typedef std::vector<Joint> JointList;

/**
* An animation data.
*
* It hold one animation data.
*/
struct Animation {
  struct Element {
	RotTrans pose;
	GLfloat time;
  };
  typedef std::pair<int/* index of the target joint */, std::vector<Element>/* the animation sequence */ > ElementList;
  typedef std::pair<const Element*, const Element*> ElementPair;

  std::string id;
  GLfloat totalTime;
  bool loopFlag;
  std::vector<ElementList> data;

  ElementPair GetElementByTime(int index, float t) const;
};

/**
* This namespace includes the polygon mesh structures and its importer.
*/
namespace Mesh {

  /**
  * The geometry of model.
  *
  * It contains some range data of any index buffer.
  * Each range is composed an offset and size.
  */
  struct Mesh {
	Mesh() {}
	Mesh(const std::string& name, int32_t offset, int32_t size) : id(name) {
	  materialList.push_back({ Material(Color4B(255, 255, 255, 255), 0, 1), offset, size });
#ifdef SHOW_TANGENT_SPACE
	  vboTBNOffset = 0;
	  vboTBNCount = 0;
#endif // SHOW_TANGENT_SPACE
	}
	void Draw() const;
	void SetJoint(const std::vector<std::string>& names, const std::vector<Joint>& joints) {
	  jointNameList = names;
	  jointList = joints;
	}

	struct MeshMaterial {
	  Material material;
	  int32_t iboOffset;
	  int32_t iboSize;
	};

	std::vector<MeshMaterial> materialList;
	std::string id;
	std::vector<std::string> jointNameList;
	JointList jointList;
	Texture::TexturePtr texDiffuse;
	Texture::TexturePtr texNormal;
#ifdef SHOW_TANGENT_SPACE
	int32_t vboTBNOffset;
	int32_t vboTBNCount;
#endif // SHOW_TANGENT_SPACE
  };

  /**
  * The result value of ImportMesh().
  */
  enum class Result {
	success,
	invalidHeader,
	noData,
	invalidMeshInfo,
	invalidIBO,
	invalidVBO,
	invalidJointInfo,
	invalidAnimationInfo,
	indexOverflow,
  };

  /**
  * The result type of ImportMesh().
  *
  * User can check up a result of importing by referring \e result member.
  * \e mesh and \e animations have a valid data, if the importing is succeeded.
  *
  * @sa ImportMesh()
  */
  struct ImportMeshResult {
	explicit ImportMeshResult(Result r) : result(r) {}

	Result result;
	std::vector<Mesh> meshes;
	std::vector<Animation> animations;
  };

#ifdef SHOW_TANGENT_SPACE
  ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd, GLuint vboTBN, GLintptr& vboTBNEnd);
#else
  ImportMeshResult ImportMesh(const RawBuffer& data, GLuint& vbo, GLintptr& vboEnd, GLuint& ibo, GLintptr& iboEnd);
#endif // SHOW_TANGENT_SPACE

  /**
  * The vertex data structure for Geometry.
  */
  struct SimpleVertex {
	Position3F  position; ///< ���_���W. 12
	Vector3F    normal; ///< ���_�m�[�}��. 12
	Vector4F    tangent; ///< ���_�^���W�F���g. 16
  };

  /**
  * The geometry.
  *
  * This structure contains only vertex and polygon index.
  * For example, it can represent the location information.
  */
  struct Geometry {
	size_t GetPolygonCount() const { return indexList.size() / 3; }

	std::string id;
	std::vector<SimpleVertex> vertexList; /// A collection of all vertices in the geometry.
	std::vector<GLushort> indexList; /// A collection of index of three vertices of a triangle.
  };

  /**
  * The result type of ImportGeometry().
  *
  * User can check up a result of importing by referring \e result member.
  * \e geometryList have a valid data, if the importing is succeeded.
  *
  * @sa ImportGeometry()
  */
  struct ImportGeometryResult {
	explicit ImportGeometryResult(Result r) : result(r) {}

	Result result;
	std::vector<Geometry> geometryList;
  };
  ImportGeometryResult ImportGeometry(const RawBuffer& data);

} // namespace Mesh
} // namespace Mai

#endif // FBX_H_INCLUDED