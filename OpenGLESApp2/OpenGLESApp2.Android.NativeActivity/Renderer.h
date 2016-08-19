#ifndef MT_RENDERER_H_INCLUDED
#define MT_RENDERER_H_INCLUDED

#include "../../Shared/Vector.h"
#include "../../Shared/Quaternion.h"
#include "../../Shared/Matrix.h"
#include "texture.h"
#include "Mesh.h"
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
//#define USE_ALPHA_TEST_IN_SHADOW_RENDERING

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

#ifdef SHOW_TANGENT_SPACE
  /**
  * 頂点のTBN(Tangent/Bitangent/Normal)確認用の頂点データ.
  */
  struct TBNVertex {
	TBNVertex(const Position3F& p, Color4B c, const uint8_t* b, const uint8_t* w)
	  : position(p)
	  , color(c)
	  , weight{ w[0], w[1], w[2], w[3] }
	  , boneID{ b[0], b[1], b[2], b[3] }
	{}
	Position3F position;
	Color4B color;
	GLubyte weight[4];
	GLubyte boneID[4];
  };

  /// TBN確認用の頂点データバッファのバイトサイズ.
  static const size_t vboTBNBufferSize = sizeof(TBNVertex) * 1024 * 11 * 3 * 2;
#endif // SHOW_TANGENT_SPACE

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

	GLint cloudColor;

	GLint fontOutlineInfo;
	GLint fontDropShadowInfo;

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

  /// シーンの地形.
  enum LandscapeOfScene {
	LandscapeOfScene_Default,
	LandscapeOfScene_Coast,
  };

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
	Object(Renderer* r, const RotTrans& rt, const Mesh::Mesh* m, const ::Mai::Material& mat, const ::Mai::Shader* s, ShadowCapability sc = ShadowCapability::Enable)
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
	const Mesh::Mesh* GetMesh() const;
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

	/**
	* Font rendering options.
	* @sa AddString()
	*/
	enum FontOption {
	  FONTOPTION_NONE = 0x00,
	  FONTOPTION_OUTLINE = 0x01,
	  FONTOPTION_DROPSHADOW = 0x02,
	  FONTOPTION_KEEPCOLOR = 0x04,
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
	void LoadLandscape(LandscapeOfScene, TimeOfScene);
	void UnloadLandscape();

	const Mesh::Mesh* GetMesh(const std::string& id) const;
	const Shader* GetShader(const std::string& id) const;

	void ClearDebugString() { debugStringList.clear(); }
	void AddDebugString(int x, int y, const char* s) { debugStringList.push_back(DebugStringObject(x, y, s)); }
	void AddString(float x, float y, float scale, const Color4B& color, const char*, int = FONTOPTION_NONE, float uw=0.0f);
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
	bool DoesDrawSkybox() const { return doesDrawSkybox; }
	void DoesDrawSkybox(bool b) { doesDrawSkybox = b; }
	void SetBlurScale(float f) { blurScale = f; }

  private:
	/** The index for identifying each FBO.

	  @sa FBOInfo, GetFBOInfo()
	*/
	enum FBOIndex {
	  // Entity
	  FBO_Main_Internal, ///< Internal only. Use FBO_Main or FBO_Shadow instead of this.
	  FBO_Sub0, /// It has 1/4 reduced size from FBO_Main.
	  FBO_Sub1, /// It has 1/4 reduced size from FBO_Main.
	  FBO_Shadow1, ///< For bluring shadow.
	  FBO_HDR0, ///< For HDR bloom effect.
	  FBO_HDR1, ///< For HDR bloom effect.
	  FBO_HDR2, ///< For HDR bloom effect.
	  FBO_HDR3, ///< For HDR bloom effect.
	  FBO_HDR4, ///< For HDR bloom effect.
	  FBO_HDR5, ///< For HDR bloom effect.

	  // Alias
	  FBO_Main, ///< For rendering color.
	  FBO_Shadow, /// For rendering shadow.
	  FBO_Sub, /// Sub buffer in the current frame. It is selected either FBO_Sub0 and FBO_Sub1 by isOddFrame.
	  FBO_Sub_Previous, /// Sub buffer in the previouse frame. It is selected either FBO_Sub0 and FBO_Sub1 by isOddFrame.

	  FBO_Begin = FBO_Main_Internal, ///< The index of first fbo entity.
	  FBO_End = FBO_HDR5 + 1, ///< The next index of last fbo entity.
	  FBO_HDR_Begin = FBO_HDR0, ///< The index of first fbo entity for bloom effect.
	  FBO_HDR_End = FBO_HDR5 + 1, ///< The next index of last fbo entity for bloom effect.

	};

	/// FBO information.
	struct FBOInfo {
	  const char* const name; ///< The texture name of FBO.
	  uint16_t width; ///< The viewport width.
	  uint16_t height; ///< The viewport height.
	  GLuint* p; ///< The pointer to FBO identification variable.
	};

	FBOInfo GetFBOInfo(int) const;
	void LoadFBX(const char* filename, const char* diffuse, const char* normal, bool showTBN = false);
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
	bool doesDrawSkybox;
	bool hasIBLTextures;
	bool isAdreno205; ///< Adreno 205 has only poor pixel fill rate. Thus, we must reduce the scale of render buffer.

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;
	GLint viewport[4];

	int isOddFrame;
	float blurScale;

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

	std::array<GLuint, FBO_End - FBO_Begin> fbo;
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
	  int options;
	};
	std::vector<FontRenderingInfo> fontRenderingInfoList;

	float animationTick;

	FilterMode filterMode;
	Color4B filterColor;
	float filterTimer;
	float filterTargetTime;

#ifdef SHOW_TANGENT_SPACE
	GLuint vboTBN;
	GLintptr vboTBNEnd;
#endif // SHOW_TANGENT_SPACE

	std::map<std::string, Shader> shaderList;
	std::map<std::string, Mesh::Mesh> meshList;
	std::map<std::string, Animation> animationList;
	std::map<std::string, Texture::TexturePtr> textureList;

	static const size_t iblSourceRoughnessCount = 7;
	std::array<Texture::TexturePtr, iblSourceRoughnessCount> iblSpecularSourceList;
	Texture::TexturePtr iblDiffuseSourceList;

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