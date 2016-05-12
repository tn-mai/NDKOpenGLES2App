#ifndef MT_RENDERER_H_INCLUDED
#define MT_RENDERER_H_INCLUDED

#include "texture.h"
#include "../../Shared/Vector.h"
#include "../../Shared/Quaternion.h"
#include "../../Shared/Matrix.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <boost/random/mersenne_twister.hpp>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <math.h>
#include <float.h>

namespace Mai {

  class Window;
  class Renderer;

#ifndef NDEBUG
#define SUNNYSIDEUP_DEBUG
  //#define SHOW_TANGENT_SPACE
#endif // NDEBUG
#define USE_HDR_BLOOM

#define VERTEX_TEXTURE_COUNT_MAX (2)

/**
* 頂点フォーマット
*/
  struct Vertex {
	Position3F  position; ///< 頂点座標. 12
	GLubyte     weight[4]; ///< 頂点ブレンディングの重み. 255 = 1.0として量子化した値を格納する. 4
	Vector3F    normal; ///< 頂点ノーマル. 12
	GLubyte     boneID[4]; ///< 頂点ブレンディングのボーンID. 4
	Position2S  texCoord[VERTEX_TEXTURE_COUNT_MAX]; ///< ディフューズ(withメタルネス)マップ座標, ノーマル(withラフネス)マップ座標. 8
	Vector4F    tangent; ///< 頂点タンジェント. 16

	Vertex() {}
  };

  template<typename T = int16_t, int F = 10, typename U = int32_t>
  class FixedNum {
  public:
	FixedNum() {}
	constexpr FixedNum(const FixedNum& n) : value(n.value) {}
	template<typename X> constexpr static FixedNum From(const X& n) { return FixedNum(static_cast<T>(n * fractional)); }
	template<typename X> constexpr X To() const { return static_cast<X>(value) / fractional; }
	template<typename X> void Set(const X& n) { value = static_cast<T>(n * fractional); }

	FixedNum& operator+=(FixedNum n) { value += n.value; return *this; }
	FixedNum& operator-=(FixedNum n) { value -= n.value; return *this; }
	FixedNum& operator*=(FixedNum n) { U tmp = value * n.value; value = tmp / fractional;  return *this; }
	FixedNum& operator/=(FixedNum n) { U tmp = value * fractional; tmp /= n.value; value = tmp;  return *this; }
	constexpr FixedNum operator+(FixedNum n) const { return FixedNum(value) += n; }
	constexpr FixedNum operator-(FixedNum n) const { return FixedNum(value) -= n; }
	constexpr FixedNum operator*(FixedNum n) const { return FixedNum(value) *= n; }
	constexpr FixedNum operator/(FixedNum n) const { return FixedNum(value) /= n; }

	FixedNum operator+=(int n) { value += n * fractional; return *this; }
	FixedNum operator-=(int n) { value -= n * fractional; return *this; }
	FixedNum operator*=(int n) { U tmp = value * (n * fractional); value = tmp / fractional;  return *this; }
	FixedNum operator/=(int n) { U tmp = value * fractional; tmp /= n; value = tmp / fractional;  return *this; }
	friend constexpr int operator+=(int lhs, FixedNum rhs) { return lhs += rhs.ToInt(); }
	friend constexpr int operator-=(int lhs, FixedNum rhs) { return lhs -= rhs.ToInt(); }
	friend constexpr int operator*=(int lhs, FixedNum rhs) { return lhs = (lhs * rhs.value) / rhs.fractional; }
	friend constexpr int operator/=(int lhs, FixedNum rhs) { return lhs = (lhs * rhs.fractional) / rhs.value; }
	friend constexpr FixedNum operator+(FixedNum lhs, int rhs) { return FixedNum(lhs) += rhs; }
	friend constexpr FixedNum operator-(FixedNum lhs, int rhs) { return FixedNum(lhs) -= rhs; }
	friend constexpr FixedNum operator*(FixedNum lhs, int rhs) { return FixedNum(lhs) *= rhs; }
	friend constexpr FixedNum operator/(FixedNum lhs, int rhs) { return FixedNum(lhs) /= rhs; }
	friend constexpr int operator+(int lhs, FixedNum rhs) { return lhs + rhs.ToInt(); }
	friend constexpr int operator-(int lhs, FixedNum rhs) { return lhs - rhs.ToInt(); }
	friend constexpr int operator*(int lhs, FixedNum rhs) { return (lhs * rhs) / rhs.fractional; }
	friend constexpr int operator/(int lhs, FixedNum rhs) { return (lhs  * rhs.fractional) / rhs; }

	static const int fractional = 2 << F;

  private:
	constexpr FixedNum(int n) : value(n) {}
	T value;
  };
  typedef FixedNum<> Fixed16;

  /**
  * モデルの材質.
  */
  struct Material {
	Color4B color;
	Fixed16 metallic;
	Fixed16 roughness;
	Material() {}
	constexpr Material(Color4B c, GLfloat m, GLfloat r) : color(c), metallic(Fixed16::From(m)), roughness(Fixed16::From(r)) {}
  };

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
	1. 各ジョイントの現在の姿勢を計算する(initialにアニメーション等による行列を掛ける). この行列はジョイントローカルな変換行列となる.
	2. ジョイントローカルな変換行列に、自身からルートジョイントまでの全ての親の姿勢行列を掛けることで、モデルローカルな変換行列を得る.
	3. モデルローカルな変換行列に逆バインドポーズ行列を掛け、最終的な変換行列を得る.
	   この変換行列は、モデルローカル座標系にある頂点をジョイントローカル座標系に移動し、
	   現在の姿勢によって変換し、再びモデルローカル座標系に戻すという操作を行う.

	逆バインドポーズ行列 = Inverse(初期姿勢行列)
  */
  struct Joint {
	RotTrans invBindPose;
	RotTrans initialPose;
	int offChild; ///< 0: no child.
	int offSibling; ///< 0: no sibling.
  };
  typedef std::vector<Joint> JointList;

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

  struct AnimationPlayer {
	AnimationPlayer() : currentTime(0), pAnime(nullptr) {}
	void SetAnimation(const Animation* p) { pAnime = p; currentTime = 0.0f; }
	void SetCurrentTime(float t) { currentTime = t; }
	float GetCurrentTime() const { return currentTime; }
	std::vector<RotTrans> Update(const JointList& jointList, float t);
	float currentTime;
	const Animation* pAnime;
	std::string id;
  };

  /**
  * モデルの幾何形状.
  * インデックスバッファ内のデータ範囲を示すオフセットとサイズを保持している.
  */
  struct Mesh {
	Mesh() {}
	Mesh(const std::string& name, int32_t offset, int32_t size) : iboOffset(offset), iboSize(size), id(name) {}
	void SetJoint(const std::vector<std::string>& names, const std::vector<Joint>& joints) {
	  jointNameList = names;
	  jointList = joints;
	}

	int32_t iboOffset;
	int32_t iboSize;
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

  /** シェーダの種類.
  */
  enum class ShaderType : int {
	Simple3D,
	Complex3D,
  };


  /**
  * シェーダ制御情報.
  */
  struct Shader
  {
	Shader() : program(0), type(ShaderType::Complex3D) {}
	~Shader();

	GLuint program;

	GLint eyePos;
	GLint lightColor;
	GLint lightPos;
	GLint materialColor;
	GLint materialMetallicAndRoughness;
	GLint dynamicRangeFactor;

	GLint texDiffuse;
	GLint texNormal;
	GLint texMetalRoughness;
	GLint texIBL;
	GLint texShadow;
	GLint texSource;

	GLint unitTexCoord;

	GLint matView;
	GLint matProjection;
	GLint bones;

	GLint lightDirForShadow;
	GLint matLightForShadow;

	GLint debug;

	ShaderType type;

	std::string id;
  };

  enum VertexAttribLocation {
	VertexAttribLocation_Position,
	VertexAttribLocation_Normal,
	VertexAttribLocation_Tangent,
	VertexAttribLocation_TexCoord01,
	VertexAttribLocation_Weight,
	VertexAttribLocation_BoneID,
	VertexAttribLocation_Color,
	VertexAttribLocation_Max,
  };

  /// シーンの時刻設定.
  enum TimeOfScene {
    TimeOfScene_Noon, ///< 昼.
    TimeOfScene_Sunset, ///< 夕方.
    TimeOfScene_Night, ///< 夜.
  };
  static const size_t TimeOfScene_Count = 3;
  Vector3F GetSunRayDirection(TimeOfScene);

  /// 影を生成する能力の有無.
  enum class ShadowCapability : int8_t {
	Disable, ///< Shadow passで描画されない.
	Enable, ///< Shadow passで描画される.
	ShadowOnly, ///< Shadow passでのみ描画される.
  };

  /**
  * 描画用オブジェクト.
  */
  class Object
  {
  public:
	Object() : isValid(false) {}
	Object(Renderer* r, const RotTrans& rt, const ::Mai::Mesh* m, const ::Mai::Material& mat, const ::Mai::Shader* s, ShadowCapability sc = ShadowCapability::Enable)
	  : shadowCapability(sc)
	  , isValid(m && s)
	  , pRenderer(r)
	  , material(mat)
	  , meshId(m ? m->id : "")
	  , shaderId(s ? s->id : "")
	  , rotTrans(rt)
	  , scale(Vector3F(1, 1, 1))
	{
	  if (m) {
		bones.resize(m->jointList.size(), Matrix4x3::Unit());
	  }
	}
	void Color(Color4B c) { material.color = c; }
	Color4B Color() const { return material.color; }
	float Metallic() const { return material.metallic.To<float>(); }
	float Roughness() const { return material.roughness.To<float>(); }
	void SetRoughness(float r) { material.roughness.Set(r); }
	void SetMetallic(float r) { material.metallic.Set(r); }
	const ::Mai::RotTrans& RotTrans() const { return rotTrans; }
	const ::Mai::Mesh* GetMesh() const;
	const ::Mai::Shader* GetShader() const;
	const GLfloat* GetBoneMatirxArray() const { return bones[0].f; }
	size_t GetBoneCount() const { return bones.size(); }
	bool IsValid() const { return isValid; }
	void Update(float t);
	void SetAnimation(const Animation* p) {
	  animationPlayer.SetAnimation(p);
	  animationPlayer.id = p ? p->id : "";
	}
	void SetCurrentTime(float t) { animationPlayer.SetCurrentTime(t); }
	float GetCurrentTime() const { return animationPlayer.GetCurrentTime(); }
	void SetRotation(const Quaternion& r) { rotTrans.rot = r; }
	void SetRotation(float x, float y, float z) {
	  (Matrix4x4::RotationZ(z) * Matrix4x4::RotationY(y) * Matrix4x4::RotationX(x)).Decompose(&rotTrans.rot, nullptr, nullptr);
	}
	void SetTranslation(const Vector3F& t) { rotTrans.trans = t; }
	void SetScale(const Vector3F& s) { scale = s; }
	Position3F Position() const { return rotTrans.trans.ToPosition3F(); }
	const Vector3F& Scale() const { return scale; }

  public:
	ShadowCapability shadowCapability;

  private:
	bool isValid;
	Renderer* pRenderer;
	Material material;
	std::string meshId;
	std::string shaderId;

	::Mai::RotTrans rotTrans;
	Vector3F scale;
	AnimationPlayer animationPlayer;
	std::vector<Matrix4x3>  bones;
  };
  typedef std::shared_ptr<Object> ObjectPtr;

  class DebugStringObject
  {
  public:
	DebugStringObject(int x, int y, const char* s) : pos(x, y), str(s) {}
	Position2S pos;
	std::string str;
  };

  /**
  * 描画処理クラス.
  */
  class Renderer
  {
  public:
	/// the color filter application mode.
	enum FilterMode {
	  FILTERMODE_NONE, ///< no color filter.
	  FILTERMODE_FADEIN, ///< fade in.
	  FILTERMODE_FADEOUT, ///< fade out.
	};

  public:
	Renderer();
	~Renderer();
	ObjectPtr CreateObject(const char* meshName, const Material& m, const char* shaderName, ShadowCapability = ShadowCapability::Enable);
	const Animation* GetAnimation(const char* name);
	void Initialize(const Window&);
	void Render(const ObjectPtr*, const ObjectPtr*);
	void Update(float dTime, const Position3F&, const Vector3F&, const Vector3F&);
	void Unload();
	void InitMesh();
	void InitTexture();

	const Mesh* GetMesh(const std::string& id) const;
	const Shader* GetShader(const std::string& id) const;

	void ClearDebugString() { debugStringList.clear(); }
	void AddDebugString(int x, int y, const char* s) { debugStringList.push_back(DebugStringObject(x, y, s)); }
	void AddString(float x, float y, float scale, const Color4B& color, const char*, float uw=0.0f);
	float GetStringWidth(const char*) const;
	float GetStringHeight(const char*) const;

	bool HasDisplay() const { return display != nullptr; }
	bool HasSurface() const { return surface != nullptr; }
	bool HasContext() const { return context != nullptr; }
	int32_t Width() const { return width; }
	int32_t Height() const { return height; }
	void Swap();

    void SetTimeOfScene(TimeOfScene toc) {
      if (toc >= 0 && toc < 3) {
        timeOfScene = toc;
      }
    }
    TimeOfScene GetTimeOfScene() const { return timeOfScene; }
	void SetFilterColor(const Color4B&);
	const Color4B& GetFilterColor() const;
	void FadeOut(const Color4B&, float);
	void FadeIn(float);
	FilterMode GetCurrentFilterMode() const;

	void SetShadowLight(const Position3F& pos, const Vector3F& dir, float n, float f, Vector2F s) {
	  shadowLightPos = pos;
	  shadowLightDir = dir;
	  shadowNear = n;
	  shadowFar = f;
	  shadowScale = s;
	}
	Position3F GetShadowLightPos() const { return shadowLightPos; }
	Vector3F GetShadowLightDir() const { return shadowLightDir; }
	float GetShadowNear() const { return shadowNear; }
	float GetShadowFar() const { return shadowFar; }
	Vector2F GetShadowMapScale() const { return shadowScale; }

  private:
	enum FBOIndex {
	  FBO_Main,
	  FBO_Sub,
	  FBO_Shadow0,
	  FBO_Shadow1,
	  FBO_HDR0,
	  FBO_HDR1,
	  FBO_HDR2,
	  FBO_HDR3,
	  FBO_HDR4,

	  FBO_End,
	};
	struct FBOInfo {
	  const char* const name;
	  GLuint* p;
	};
	FBOInfo GetFBOInfo(int) const;
	void LoadFBX(const char* filename, const char* diffuse, const char* normal);
	void CreateSkyboxMesh();
	void CreateUnitBoxMesh();
	void CreateOctahedronMesh();
	void CreateBoardMesh(const char*, const Vector3F&);
	void CreateFloorMesh(const char*, const Vector3F&, int);
	void CreateAsciiMesh(const char*);
	void CreateCloudMesh(const char*, const Vector3F&);
	void DrawFont(const Position2F&, const char*);
	void DrawFontFoo();

  private:
	bool isInitialized;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	GLint viewport[4];

	std::string texBaseDir;

	boost::random::mt19937 random;

    TimeOfScene timeOfScene;
	Position3F shadowLightPos;
	Vector3F shadowLightDir;
	float shadowNear;
	float shadowFar;
	Vector2F shadowScale;

	Position3F cameraPos;
	Vector3F cameraDir;
	Vector3F cameraUp;

	GLuint fboMain, fboSub, fboShadow0, fboShadow1, fboHDR[5];
	GLuint depth;

	GLuint vbo;
	GLintptr vboEnd;
	GLuint ibo;
	GLintptr iboEnd;

	GLuint vboFont[2];
	GLintptr vboFontEnd;
	struct FontRenderingInfo {
	  GLint first;
	  GLsizei count;
	};
	std::vector<FontRenderingInfo> fontRenderingInfoList;
	int currentFontBufferNo;

	FilterMode filterMode;
	Color4B filterColor;
	float filterTimer;
	float filterTargetTime;

#ifdef SHOW_TANGENT_SPACE
	GLuint vboTBN;
	GLintptr vboTBNEnd;
#endif // SHOW_TANGENT_SPACE

	std::map<std::string, Shader> shaderList;
	std::map<std::string, Mesh> meshList;
	std::map<std::string, Animation> animationList;
	std::map<std::string, Texture::TexturePtr> textureList;

	static const size_t iblSourceRoughnessCount = 7;
	std::array<std::array<Texture::TexturePtr, iblSourceRoughnessCount>, TimeOfScene_Count> iblSpecularSourceList;
	std::array<Texture::TexturePtr, TimeOfScene_Count> iblDiffuseSourceList;

	std::vector<DebugStringObject> debugStringList;
  };


  Matrix4x4 LookAt(const Position3F& eyePos, const Position3F& targetPos, const Vector3F& upVector);
  inline uint8_t FloatToFix8(float f) { return static_cast<uint8_t>(std::max<float>(0, std::min<float>(255, f * 255.0f + 0.5f))); }
  inline float Fix8ToFloat(uint8_t n) { return static_cast<float>(n) * (1.0f / 255.0f); }

  template<typename T> constexpr T Normalize(const T v) { return v * (1.0f / v.Length()); }
  template<typename T> constexpr T Cross(const T& lhs, const T& rhs) { return lhs.Cross(rhs); }
  template<typename T> constexpr float Dot(const T& lhs, const T& rhs) { return lhs.Dot(rhs); }

} // namespace Mai

#endif // MT_RENDERER_H_INCLUDED