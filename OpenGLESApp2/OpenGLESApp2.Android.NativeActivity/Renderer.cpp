#include "Renderer.h"
#include "../../Shared/File.h"
#include "../../Shared/Window.h"
#include "../../Shared/FontInfo.h"
#ifdef __ANDROID__
#include "android_native_app_glue.h"
#include <android/log.h>
#endif // __ANDROID__

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <boost/optional.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <vector>
#include <numeric>
#include <math.h>
#include <chrono>

namespace Mai {

static const Vector2F referenceViewportSize(480, 800);

#ifdef __ANDROID__
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "AndroidProject1.NativeActivity", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

static const size_t vboBufferSize = sizeof(Vertex) * 1024 * 11;
static const size_t iboBufferSize = sizeof(GLushort) * 1024 * 10 * 3;
#define MAX_FONT_RENDERING_COUNT 512

namespace {

  namespace Local {
	PFNGLDELETEFENCESNVPROC glDeleteFencesNV;
	PFNGLGENFENCESNVPROC glGenFencesNV;
	PFNGLGETFENCEIVNVPROC glGetFenceivNV;
	PFNGLISFENCENVPROC glIsFenceNV;
	PFNGLFINISHFENCENVPROC glFinishFenceNV;
	PFNGLSETFENCENVPROC glSetFenceNV;
	PFNGLTESTFENCENVPROC glTestFenceNV;
  }
  GLvoid GL_APIENTRY dummy_glDeleteFencesNV(GLsizei, const GLuint*) {}
  GLvoid GL_APIENTRY dummy_glGenFencesNV(GLsizei, GLuint*) {}
  GLvoid GL_APIENTRY dummy_glGetFenceivNV(GLuint, GLenum, GLint*) {}
  GLboolean GL_APIENTRY dummy_glIsFenceNV(GLuint) { return false; }
  GLvoid GL_APIENTRY dummy_glFinishFenceNV(GLuint) {}
  GLvoid GL_APIENTRY dummy_glSetFenceNV(GLuint, GLenum) {}
  GLboolean GL_APIENTRY dummy_glTestFenceNV(GLuint) { return false; }

  void InitNVFenceExtention(bool hasNVfenceExtension)
  {
#ifndef NDEBUG
	if (hasNVfenceExtension) {
	  Local::glDeleteFencesNV = (PFNGLDELETEFENCESNVPROC)eglGetProcAddress("glDeleteFencesNV");
	  Local::glGenFencesNV = (PFNGLGENFENCESNVPROC)eglGetProcAddress("glGenFencesNV");
	  Local::glGetFenceivNV = (PFNGLGETFENCEIVNVPROC)eglGetProcAddress("glGetFenceivNV");
	  Local::glIsFenceNV = (PFNGLISFENCENVPROC)eglGetProcAddress("glIsFenceNV");
	  Local::glFinishFenceNV = (PFNGLFINISHFENCENVPROC)eglGetProcAddress("glFinishFenceNV");
	  Local::glSetFenceNV = (PFNGLSETFENCENVPROC)eglGetProcAddress("glSetFenceNV");
	  Local::glTestFenceNV = (PFNGLTESTFENCENVPROC)eglGetProcAddress("glTestFenceNV");
	  LOGI("Enable GL_NV_fence");
	} else {
	  Local::glDeleteFencesNV = dummy_glDeleteFencesNV;
	  Local::glGenFencesNV = dummy_glGenFencesNV;
	  Local::glGetFenceivNV = dummy_glGetFenceivNV;
	  Local::glIsFenceNV = dummy_glIsFenceNV;
	  Local::glFinishFenceNV = dummy_glFinishFenceNV;
	  Local::glSetFenceNV = dummy_glSetFenceNV;
	  Local::glTestFenceNV = dummy_glTestFenceNV;
	  LOGI("Disable GL_NV_fence");
	}
#else
	Local::glDeleteFencesNV = dummy_glDeleteFencesNV;
	Local::glGenFencesNV = dummy_glGenFencesNV;
	Local::glGetFenceivNV = dummy_glGetFenceivNV;
	Local::glIsFenceNV = dummy_glIsFenceNV;
	Local::glFinishFenceNV = dummy_glFinishFenceNV;
	Local::glSetFenceNV = dummy_glSetFenceNV;
	Local::glTestFenceNV = dummy_glTestFenceNV;
	LOGI("Disable GL_NV_fence");
#endif // NDEBUG
  }

  int64_t GetCurrentTime()
  {
#ifdef __ANDROID__
#if 1
	timespec tmp;
	clock_gettime(CLOCK_MONOTONIC, &tmp);
	return tmp.tv_sec * (1000UL * 1000UL * 1000UL) + tmp.tv_nsec;
#elif 0
	struct timeval tmp;
	gettimeofday(&tmp, nullptr);
	return tmp.tv_sec * 1000UL * 1000UL * 1000UL + tmp.tv_usec * 1000UL;
#endif
#else
	const auto t = std::chrono::high_resolution_clock::now();
	return std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
#endif // __ANDROID__
  }

  template<typename T, typename F>
  std::vector<T> split(const std::string& str, char delim, F func) {
	std::vector<T> v;
	std::istringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delim)) {
	  v.push_back(static_cast<T>(func(item)));
	}
	return v;
  }
  
  template<typename T>
	T degreeToRadian(T d) {
		return d * boost::math::constants::pi<T>() / static_cast<T>(180);
	}

	void checkGlError(const char* op) {
		for (GLint error = glGetError(); error; error = glGetError()) {
			LOGI("after %s() glError (0x%x)\n", op, error);
		}
	}

	GLuint LoadShader(GLenum shaderType, const char* path, const std::string& additionalDefineList) {
		GLuint shader = 0;
		if (auto buf = FileSystem::LoadFile(path)) {
			static const GLchar version[] = "#version 100\n";
			static const GLchar defineList[] =
#ifdef SUNNYSIDEUP_DEBUG
			  "#define DEBUG\n"
#endif // SUNNYSIDEUP_DEBUG
#ifdef USE_HDR_BLOOM
			  "#define USE_HDR_BLOOM\n"
#endif // USE_HDR_BLOOM
#ifdef USE_ALPHA_TEST_IN_SHADOW_RENDERING
			  "#define USE_ALPHA_TEST_IN_SHADOW_RENDERING\n"
#endif // USE_ALPHA_TEST_IN_SHADOW_RENDERING
			  "#define SCALE_BONE_WEIGHT(w) ((w) * (1.0 / 255.0))\n"
			  "#define SCALE_TEXCOORD(c) ((c) * (1.0 / 65535.0))\n"
			  "#define M_PI (3.1415926535897932384626433832795)\n"
			  ;
			shader = glCreateShader(shaderType);
			if (!shader) {
				return 0;
			}
			const GLchar* pSrc[] = {
			  version,
			  defineList,
			  additionalDefineList.data(),
			  reinterpret_cast<GLchar*>(static_cast<void*>(&(*buf)[0])),
			};
			const GLint srcSize[] = {
			  sizeof(version) - 1,
			  sizeof(defineList) - 1,
			  static_cast<GLint>(additionalDefineList.size()),
			  static_cast<GLint>(buf->size()),
			};
			glShaderSource(shader, sizeof(pSrc)/sizeof(pSrc[0]), pSrc, srcSize);
			glCompileShader(shader);
		}
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				std::vector<char> buf;
				buf.resize(infoLen);
				if (static_cast<int>(buf.size()) >= infoLen) {
					glGetShaderInfoLog(shader, infoLen, NULL, &buf[0]);
					LOGE("Could not compile shader %d:\n%s\n", shaderType, &buf[0]);
				}
				glDeleteShader(shader);
				return 0;
			}
		}
		return shader;
	}

	boost::optional<Shader> CreateShaderProgram(const char* name, const char* vshPath, const char* fshPath, const std::string& additionalDefineList) {
		GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vshPath, additionalDefineList);
		if (!vertexShader) {
			return boost::none;
		}

		GLuint pixelShader = LoadShader(GL_FRAGMENT_SHADER, fshPath, additionalDefineList);
		if (!pixelShader) {
			return boost::none;
		}

		GLuint program = glCreateProgram();
		if (program) {
			glAttachShader(program, vertexShader);
			checkGlError("glAttachShader");
			glAttachShader(program, pixelShader);
			checkGlError("glAttachShader");
			glBindAttribLocation(program, VertexAttribLocation_Position, "vPosition");
			glBindAttribLocation(program, VertexAttribLocation_Normal, "vNormal");
			glBindAttribLocation(program, VertexAttribLocation_Tangent, "vTangent");
			glBindAttribLocation(program, VertexAttribLocation_TexCoord01, "vTexCoord01");
			glBindAttribLocation(program, VertexAttribLocation_Weight, "vWeight");
			glBindAttribLocation(program, VertexAttribLocation_BoneID, "vBoneID");
			glBindAttribLocation(program, VertexAttribLocation_Color, "vColor");
			glLinkProgram(program);
			GLint linkStatus = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
			if (linkStatus != GL_TRUE) {
				GLint bufLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
				if (bufLength) {
					char* buf = (char*)malloc(bufLength);
					if (buf) {
						glGetProgramInfoLog(program, bufLength, NULL, buf);
						LOGE("Could not link program:\n%s\n", buf);
						free(buf);
					}
				}
				glDeleteProgram(program);
				return boost::none;
			}
		}
		Shader s;
		s.program = program;
		s.eyePos = glGetUniformLocation(program, "eyePos");
		s.lightColor = glGetUniformLocation(program, "lightColor");
		s.lightPos = glGetUniformLocation(program, "lightPos");
		s.materialColor = glGetUniformLocation(program, "materialColor");
		s.materialMetallicAndRoughness = glGetUniformLocation(program, "metallicAndRoughness");
		s.dynamicRangeFactor = glGetUniformLocation(program, "dynamicRangeFactor");
		s.texDiffuse = glGetUniformLocation(program, "texDiffuse");
		s.texNormal = glGetUniformLocation(program, "texNormal");
		s.texMetalRoughness = glGetUniformLocation(program, "texMetalRoughness");
		s.texIBL = glGetUniformLocation(program, "texIBL");
		s.texShadow = glGetUniformLocation(program, "texShadow");
		s.texSource = glGetUniformLocation(program, "texSource");
		s.unitTexCoord = glGetUniformLocation(program, "unitTexCoord");
		s.matView = glGetUniformLocation(program, "matView");
		s.matProjection = glGetUniformLocation(program, "matProjection");
		s.bones = glGetUniformLocation(program, "boneMatrices");
		s.lightDirForShadow = glGetUniformLocation(program, "lightDirForShadow");
		s.matLightForShadow = glGetUniformLocation(program, "matLightForShadow");
		s.cloudColor = glGetUniformLocation(program, "cloudColor");
		s.fontOutlineInfo = glGetUniformLocation(program, "fontOutlineInfo");
		s.fontDropShadowInfo = glGetUniformLocation(program, "fontDropShadowInfo");

		s.debug = glGetUniformLocation(program, "debug");

		s.id = name;
		return s;
	}

	/** 射影行列を作成する.
	  gluPerspectiveと同じ行列が作成される.
	*/
	Matrix4x4 Perspective(float fov, float width, float height, float nearPlane, float farPlane)
	{
		const float ar = width / height;
		const float tanHalfFov = std::tan(degreeToRadian(fov / 2.0f));
		const float F = 1.0f / tanHalfFov;
		const float	rcpnmf = 1.0f / (nearPlane - farPlane);
		Matrix4x4 m;
		m.SetVector(0, Vector4F(F / ar, 0, 0, 0));
		m.SetVector(1, Vector4F(0, F, 0, 0));
		m.SetVector(2, Vector4F(0, 0, (nearPlane + farPlane) * rcpnmf, -1.0f));
		m.SetVector(3, Vector4F(0, 0, (2.0f * farPlane * nearPlane) * rcpnmf, 0));
		return m;
	}

	Matrix4x4 Olthographic(float width, float height, float nearPlane, float farPlane)
	{
	  Matrix4x4 m;
	  m.SetVector(0, Vector4F(2.0f / width, 0, 0, 0));
	  m.SetVector(1, Vector4F(0.0f, 2.0f / height, 0, 0));
	  m.SetVector(2, Vector4F(0, 0, -2.0f / (farPlane - nearPlane), 0));
	  m.SetVector(3, Vector4F(0.0f, 0.0f, -(farPlane + nearPlane) / (farPlane - nearPlane), 1.0f));
	  return m;
	}

} // unnamed namespace

Matrix4x4 LookAt(const Position3F& eyePos, const Position3F& targetPos, const Vector3F& upVector)
{
  const Vector3F ezVector = (eyePos - targetPos).Normalize();
  const Vector3F exVector = upVector.Cross(ezVector).Normalize();
  const Vector3F eyVector = ezVector.Cross(exVector).Normalize();
  Matrix4x4 m;
  m.SetVector(0, Vector4F(exVector.x, eyVector.x, ezVector.x, 0));
  m.SetVector(1, Vector4F(exVector.y, eyVector.y, ezVector.y, 0));
  m.SetVector(2, Vector4F(exVector.z, eyVector.z, ezVector.z, 0));
  m.SetVector(3, Vector4F(-Dot(exVector, eyePos), -Dot(eyVector, eyePos), -Dot(ezVector, eyePos), 1.0f));
  return m;
}

/** Shaderデストラクタ.
*/
Shader::~Shader()
{
}

/** RotTransを4x3行列に変換する.
*/
Matrix4x3 ToMatrix(const RotTrans& rt) {
	Matrix4x3 m = ToMatrix(rt.rot);
	m.Set(0, 3, rt.trans.x);
	m.Set(1, 3, rt.trans.y);
	m.Set(2, 3, rt.trans.z);
	return m;
}

/** クォータニオンの球面線形補間

  @ref http://www.willperone.net/Code/quaternion.php
*/
Quaternion Sleap(const Quaternion& qa, const Quaternion& qb, float t)
{
	Quaternion tmp;
	float cosHalfTheta = qa.Dot(qb);
	if (cosHalfTheta < 0.0f) {
	  cosHalfTheta = -cosHalfTheta;
	  tmp = -qb;
	} else {
	  tmp = qb;
	}
	if (cosHalfTheta < 0.95f) {
	  const float halfTheta = std::acos(cosHalfTheta);
	  const float sinHalfTheta = std::sin(halfTheta);
	  const float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
	  const float ratioB = std::sin(t * halfTheta) / sinHalfTheta;
	  return (qa * ratioA + tmp * ratioB).Normalize();
	} else {
	  return (qa * (1.0f - t) + tmp * t).Normalize();
	}
}

/** 指定された時間の直前と直後にあたるアニメーションデータのペアを取得する.
*/
Animation::ElementPair Animation::GetElementByTime(int index, float t) const
{
	static const Element defaultElem = { { Quaternion(0, 0, 0, 1), Vector3F(0, 0, 0)}, 0 };

	const std::vector<ElementList>::const_iterator itrList = std::lower_bound(
		data.begin(), data.end(), index, [](const ElementList& lhs, int rhs) { return lhs.first < rhs; });
	if (itrList == data.end() || itrList->first != index) {
		return ElementPair(&defaultElem, &defaultElem);
	}
	const ElementList& list = *itrList;
	if (list.second.size() == 0) {
		return ElementPair(&defaultElem, &defaultElem);
	}
	const ElementList::second_type::const_iterator itrElem = std::upper_bound(
		list.second.begin(), list.second.end(), t, [](float lhs, const Element& rhs) { return lhs < rhs.time; });
	if (itrElem == list.second.begin()) {
		return ElementPair(&defaultElem, &*itrElem);
	} else if (itrElem == list.second.end()) {
		return ElementPair(&*(itrElem - 1), &*(itrElem - 1));
	}
	return ElementPair(&*(itrElem - 1), &*itrElem);
}

/** 指定された時間だけアニメーションを進める.
*/
std::vector<RotTrans> AnimationPlayer::Update(const JointList& jointList, float t)
{
	std::vector<RotTrans> result;
	if (!pAnime) {
		result.resize(jointList.size(), RotTrans::Unit());
		return result;
	}
	currentTime += t;
	if (pAnime->loopFlag) {
	  currentTime = std::fmod(currentTime, pAnime->totalTime);
	}
	result.reserve(jointList.size());
	for (int i = 0; i < static_cast<int>(jointList.size()); ++i) {
		const Animation::ElementPair p = pAnime->GetElementByTime(i, currentTime);
		const float timeRange = p.second->time - p.first->time;
		const float ratio = timeRange > 0.0f ? (currentTime - p.first->time) / timeRange : 0.0f;
		result.push_back(Interporation(p.first->pose, p.second->pose, ratio));
	}
	return result;
}

void UpdateJointRotTrans(std::vector<RotTrans>& rtList, const Joint* root, const Joint* joint, const RotTrans& parentRT)
{
	RotTrans& rtBuf = rtList[joint - root];
#if 0
	RotTrans rt = rtBuf * joint->initialPose;
	rt *= parentRT;
	rtBuf = rt * joint->invBindPose;
#elif 0
	rtBuf = joint->invBindPose * rtBuf;
#endif
	if (joint->offChild) {
		UpdateJointRotTrans(rtList, root, joint + joint->offChild, rtBuf);
	}
	if (joint->offSibling) {
		UpdateJointRotTrans(rtList, root, joint + joint->offSibling, parentRT);
	}
}

/** オブジェクト状態を更新する.
*/
void Object::Update(float t)
{
  if (!IsValid()) {
	return;
  }
  if (const Mesh::Mesh* mesh = GetMesh()) {
	if (!mesh->jointList.empty()) {
	  if (const Animation* pAnime = pRenderer->GetAnimation(animationPlayer.id.c_str())) {
		const Matrix4x3 m0 = ToMatrix(rotTrans) * Matrix4x3 {
		  {
			scale.x, 0, 0, 0,
			  0, scale.y, 0, 0,
			  0, 0, scale.z, 0,
		  }
		};
		animationPlayer.pAnime = pAnime;
		std::vector<Mai::RotTrans> rtList = animationPlayer.Update(mesh->jointList, t);
		UpdateJointRotTrans(rtList, &mesh->jointList[0], &mesh->jointList[0], Mai::RotTrans::Unit());
		std::transform(rtList.begin(), rtList.end(), bones.begin(), [&m0](const Mai::RotTrans& rt) {
		  const Matrix4x3 m = ToMatrix(rt);
		  return m0 * m;
		});
	  }
	}
  }
}

/** Get a mesh object.

  @return A pointer to the mesh object if it is exists in the renderer,
          otherwise nullptr.
*/
const Mesh::Mesh* Object::GetMesh() const {
  return pRenderer->GetMesh(meshId);
}

/** Get a shader object.

  @return A pointer to the shader object if it is exists in the renderer,
          otherwise nullptr.
*/
const ::Mai::Shader* Object::GetShader() const {
  return pRenderer->GetShader(shaderId);
}

/** シーンに対応する太陽光線の向きを取得する.

  @param  timeOfScene  シーンの種類.

  @return timeOfSceneに対応する太陽光線の向き.
*/
Vector3F GetSunRayDirection(TimeOfScene timeOfScene) {
  static const Vector3F baseAxis(0.25f, 0, -1.0f);
  static const Vector3F baseRay(0, -1, 0);
  static const float baseAxisAngle = 20.0f;
  static const Vector3F rightAxis(1, 0, 0);
  const Vector3F noonRay(Normalize(Quaternion(Normalize(baseAxis), degreeToRadian(baseAxisAngle)).Apply(baseRay)));
  const Vector3F axis(Normalize(Cross(noonRay, rightAxis)));

#if 0
  static int degree = 15;
  degree = (degree + 5) % 360;
  return Normalize(Quaternion(axis, degreeToRadian<float>(degree)).Apply(noonRay));
#else
  switch (timeOfScene) {
  default:
  case TimeOfScene_Noon: return Normalize(Quaternion(axis, degreeToRadian(0.0f)).Apply(noonRay));
  case TimeOfScene_Sunset: return Normalize(Quaternion(axis, degreeToRadian(75.0f)).Apply(noonRay));
  case TimeOfScene_Night: return Normalize(Quaternion(axis, degreeToRadian(45.0f)).Apply(noonRay));
  }
#endif
}

/** コンストラクタ.
*/
Renderer::Renderer()
  : isInitialized(false)
  , doesDrawSkybox(true)
  , hasIBLTextures(false)
  , isAdreno205(false)
  , width(480 * 8 / 10)
  , height(640 * 8 / 10)
  , texBaseDir("Textures/Others/")
  , random(static_cast<uint32_t>(time(nullptr)))
  , timeOfScene(TimeOfScene_Noon)
  , shadowLightPos(0, 2000, 0)
  , shadowLightDir(0, -1, 0)
  , shadowNear(10)
  ,	shadowFar(2000)
  , shadowScale(1, 1)
  , depth(0)
  , animationTick(0.0)
  , filterMode(FILTERMODE_NONE)
  , filterColor(0, 0, 0, 0)
  , filterTimer(0.0f)
{
  for (auto& e : fbo) {
	e = 0;
  }
}

/** デストラクタ.
*/
Renderer::~Renderer()
{
	Unload();
}

Renderer::FBOInfo Renderer::GetFBOInfo(int id) const
{
	static const float baseAspectRatio = 9.0f / 16.0f;
	const uint16_t MAIN_RENDERING_PATH_HEIGHT = isAdreno205 ? ((height * 8) / 10) : height;
	static const uint16_t SHADOWMAP_MAIN_WIDTH = 256;
	static const uint16_t SHADOWMAP_MAIN_HEIGHT = 1024;
	const uint16_t FBO_MAIN_HEIGHT = (MAIN_RENDERING_PATH_HEIGHT > SHADOWMAP_MAIN_HEIGHT ? MAIN_RENDERING_PATH_HEIGHT : SHADOWMAP_MAIN_HEIGHT);

	const float aspectRatio = static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
	const uint16_t MAIN_RENDERING_PATH_WIDTH = static_cast<uint16_t>(std::max(256, isAdreno205 ? ((width * 8) / 10) : width));
	const uint16_t FBO_MAIN_WIDTH = (MAIN_RENDERING_PATH_WIDTH > SHADOWMAP_MAIN_WIDTH ? MAIN_RENDERING_PATH_WIDTH : SHADOWMAP_MAIN_WIDTH);

	// This list should keep same order as a enumeration type of 'FBOIndex'.
	const struct {
		const char* name;
		FBOIndex index;
		uint16_t width;
		uint16_t height;
	} fboNameList[] = {
		// Entity
		{ "fboMain", FBO_Main_Internal, FBO_MAIN_WIDTH, FBO_MAIN_HEIGHT },
		{ "fboSub", FBO_Sub, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 4), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 4) },
		{ "fboShadow1", FBO_Shadow1, static_cast<uint16_t>(SHADOWMAP_MAIN_WIDTH), static_cast<uint16_t>(SHADOWMAP_MAIN_HEIGHT) },
		{ "fboHDR0", FBO_HDR0, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 4), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 4) },
		{ "fboHDR1", FBO_HDR1, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 4), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 4) },
		{ "fboHDR2", FBO_HDR2, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 8), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 8) },
		{ "fboHDR3", FBO_HDR3, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 16), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 16) },
		{ "fboHDR4", FBO_HDR4, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 32), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 32) },
		{ "fboHDR5", FBO_HDR5, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH / 64), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT / 64) },

		// Alias
		{ "fboMain", FBO_Main_Internal, static_cast<uint16_t>(MAIN_RENDERING_PATH_WIDTH), static_cast<uint16_t>(MAIN_RENDERING_PATH_HEIGHT) },
		{ "fboMain", FBO_Main_Internal, static_cast<uint16_t>(SHADOWMAP_MAIN_WIDTH), static_cast<uint16_t>(SHADOWMAP_MAIN_HEIGHT) },
	};

	GLuint* p = const_cast<GLuint*>(&fbo[fboNameList[id].index]);
	return { fboNameList[id].name, fboNameList[id].width, fboNameList[id].height, p };
}

/** 描画環境の初期設定.
* OpenGL環境の再構築が必要になった場合、その都度この関数を呼び出す必要がある.
*/
void Renderer::Initialize(const Window& window)
{
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	static const EGLint attribs[] = {
		EGL_BLUE_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	EGLConfig config;
	EGLint numConfigs;
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	EGLint format;
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
#ifdef __ANDROID__
	ANativeWindow_setBuffersGeometry(window.GetWindowType(), 0, 0, format);
#endif // __ANDROID__
	surface = eglCreateWindowSurface(display, config, window.GetWindowType(), nullptr);
	static const EGLint contextAttribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		LOGI("Unable to eglMakeCurrent");
		return;
	}
	eglQuerySurface(display, surface, EGL_WIDTH, &width);
	eglQuerySurface(display, surface, EGL_HEIGHT, &height);

	{
	  const char* pRendererName = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	  LOGI("GL_VENDOR: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	  LOGI("GL_RENDERER: %s", pRendererName);
	  LOGI("GL_VERSION: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	  isAdreno205 = std::strcmp(pRendererName, "Adreno (TM) 205") == 0;
	  if (isAdreno205) {
		LOGI("Detect Adreno 205");
	  }
	}

#define	LOG_SHADER_INFO(s) { \
		GLint tmp; \
		glGetIntegerv(s, &tmp); \
		LOGI(#s": %d", tmp); }
#define MAKE_TEX_ID_PAIR(s) { s, #s },

	LOGI("GL_COMPRESSED_TEXTURE_FORMATS:");
	GLint formatCount;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &formatCount);
	std::vector<GLint> formatArray;
	formatArray.resize(formatCount);
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, &formatArray[0]);
	struct { uint16_t id; const char* const str; } texInfoList[] = {
	  MAKE_TEX_ID_PAIR(GL_ETC1_RGB8_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE4_RGB8_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE4_RGBA8_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE4_R5_G6_B5_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE4_RGBA4_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE4_RGB5_A1_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE8_RGB8_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE8_RGBA8_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE8_R5_G6_B5_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE8_RGBA4_OES)
	  MAKE_TEX_ID_PAIR(GL_PALETTE8_RGB5_A1_OES)
	  MAKE_TEX_ID_PAIR(GL_DEPTH_COMPONENT24_OES)
	  MAKE_TEX_ID_PAIR(GL_DEPTH_COMPONENT32_OES)
	  MAKE_TEX_ID_PAIR(GL_TEXTURE_3D_OES)
	  MAKE_TEX_ID_PAIR(GL_HALF_FLOAT_OES)
	  MAKE_TEX_ID_PAIR(GL_3DC_X_AMD)
	  MAKE_TEX_ID_PAIR(GL_3DC_XY_AMD)
	  MAKE_TEX_ID_PAIR(GL_ATC_RGB_AMD)
	  MAKE_TEX_ID_PAIR(GL_ATC_RGBA_EXPLICIT_ALPHA_AMD)
	  MAKE_TEX_ID_PAIR(GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD)
	  MAKE_TEX_ID_PAIR(GL_BGRA_EXT)
	  MAKE_TEX_ID_PAIR(GL_UNSIGNED_INT_2_10_10_10_REV_EXT)
	  MAKE_TEX_ID_PAIR(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
	  MAKE_TEX_ID_PAIR(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
	  MAKE_TEX_ID_PAIR(GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG)
	  MAKE_TEX_ID_PAIR(GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG)
	  MAKE_TEX_ID_PAIR(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
	  MAKE_TEX_ID_PAIR(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
	};
	for (const auto id : formatArray) {
	  if (id == GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD) {
		texBaseDir.assign("Textures/Adreno/");
	  }
	  bool isLogged = false;
	  for (const auto* i = texInfoList; i != texInfoList + sizeof(texInfoList) / sizeof(texInfoList[0]); ++i) {
		if (i->id == id) {
		  LOGI("  0x%04x: %s", i->id, i->str);
		  isLogged = true;
		  break;
		}
	  }
	  if (!isLogged) {
		LOGI("  0x%04x: (Unknown format)", id);
	  }
	}

	LOG_SHADER_INFO(GL_MAX_TEXTURE_IMAGE_UNITS);
	LOG_SHADER_INFO(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);

	bool hasNVfenceExtension = false;
	GLenum depthComponentType = GL_DEPTH_COMPONENT16;
	{
	  LOGI("GL_EXTENTIONS:");
	  const char* p = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	  const std::vector<std::string> v = split<std::string>(p, ' ', [](const std::string& s) { return s; });
	  for (const auto& e : v) {
		LOGI("  %s", e.c_str());
		if (e == "GL_NV_fence") {
		  hasNVfenceExtension = true;
		}
		if (e == "GL_OES_depth32") {
		  depthComponentType = GL_DEPTH_COMPONENT32_OES;
		} else if (depthComponentType != GL_DEPTH_COMPONENT32_OES) {
		  if (e == "GL_OES_depth24") {
			depthComponentType = GL_DEPTH_COMPONENT24_OES;
		  } else if (depthComponentType != GL_DEPTH_COMPONENT24_OES) {
			if (e == "GL_OES_packed_depth_stencil") {
			  depthComponentType = GL_DEPTH24_STENCIL8_OES;
			}
		  }
		}
	  }
	  static const struct {
		GLenum id;
		const char* str;
	  } depthInfo[] = {
		{ GL_DEPTH_COMPONENT32_OES, "32" },
		{ GL_DEPTH_COMPONENT24_OES, "24" },
		{ GL_DEPTH24_STENCIL8_OES, "24/8" },
		{ GL_DEPTH_COMPONENT16, "16" },
	  };
	  for (const auto& e : depthInfo) {
		if (e.id == depthComponentType) {
		  LOGI("Depth buffer precision: %s", e.str);
		  break;
		}
	  }
	}
#undef MAKE_TEX_ID_PAIR
#undef LOG_SHADER_INFO

	InitNVFenceExtention(hasNVfenceExtension);

	glGetIntegerv(GL_VIEWPORT, viewport);
	LOGI("viewport: %dx%d", viewport[2], viewport[3]);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);

	std::ostringstream  additionalDefineList;
	{
	  const FBOInfo fboMainInternalInfo = GetFBOInfo(FBO_Main_Internal);
	  LOGI("FBO MAIN INTERNAL: %dx%d", fboMainInternalInfo.width, fboMainInternalInfo.height);
	  additionalDefineList << "#define FBO_MAIN_WIDTH (" << fboMainInternalInfo.width << ".0)\n";
	  additionalDefineList << "#define FBO_MAIN_HEIGHT (" << fboMainInternalInfo.height << ".0)\n";
	  const FBOInfo fboShadowInfo = GetFBOInfo(FBO_Shadow);
	  LOGI("FBO SHADOW: %dx%d", fboShadowInfo.width, fboShadowInfo.height);
	  additionalDefineList << "#define SHADOWMAP_MAIN_WIDTH (" << fboShadowInfo.width << ".0)\n";
	  additionalDefineList << "#define SHADOWMAP_MAIN_HEIGHT (" << fboShadowInfo.height << ".0)\n";
	  const FBOInfo fboMainInfo = GetFBOInfo(FBO_Main);
	  LOGI("FBO MAIN: %dx%d", fboMainInfo.width, fboMainInfo.height);
	  additionalDefineList << "#define MAIN_RENDERING_PATH_WIDTH (" << fboMainInfo.width << ".0)\n";
	  additionalDefineList << "#define MAIN_RENDERING_PATH_HEIGHT (" << fboMainInfo.height << ".0)\n";
	}

	static const struct {
	  ShaderType type;
	  const char* name;
	} shaderInfoList[] = {
	  { ShaderType::Complex3D, "default" },
	  { ShaderType::Complex3D, "defaultWithAlpha" },
	  { ShaderType::Complex3D, "default2D" },
	  { ShaderType::Complex3D, "cloud" },
	  { ShaderType::Simple3D, "solidmodel" },
	  { ShaderType::Simple3D, "sea" },
	  { ShaderType::Complex3D, "emission" },
	  { ShaderType::Complex3D, "skybox" },
	  { ShaderType::Complex3D, "shadow" },
	  { ShaderType::Complex3D, "bilinear4x4" },
	  { ShaderType::Complex3D, "sample4" },
	  { ShaderType::Complex3D, "reduceLum" },
	  { ShaderType::Complex3D, "hdrdiff" },
	  { ShaderType::Complex3D, "applyhdr" },
	  { ShaderType::Complex3D, "tbn" },
	  { ShaderType::Complex3D, "font" },
	};
	for (const auto e : shaderInfoList) {
		const std::string vert = std::string("Shaders/") + std::string(e.name) + std::string(".vert");
		const std::string frag = std::string("Shaders/") + std::string(e.name) + std::string(".frag");
		if (boost::optional<Shader> s = CreateShaderProgram(e.name, vert.c_str(), frag.c_str(), additionalDefineList.str())) {
			s->type = e.type;
			shaderList.insert({ s->id, *s });
		}
	}

	InitTexture();

	{
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vboBufferSize, 0, GL_STATIC_DRAW);
		vboEnd = 0;

		glGenBuffers(2, vboFont);
		for (int i = 0; i < 2; ++i) {
		  glBindBuffer(GL_ARRAY_BUFFER, vboFont[i]);
		  glBufferData(GL_ARRAY_BUFFER, sizeof(FontVertex) * 4/*rectangle*/ * MAX_FONT_RENDERING_COUNT, 0, GL_DYNAMIC_DRAW);
		}
		vboFontEnd = 0;
		currentFontBufferNo = 0;
		fontRenderingInfoList.clear();
		fontRenderingInfoList.reserve(MAX_FONT_RENDERING_COUNT / 8);

		glGenBuffers(1, &ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboBufferSize, 0, GL_STATIC_DRAW);
		iboEnd = 0;
#ifdef SHOW_TANGENT_SPACE
		glGenBuffers(1, &vboTBN);
		glBindBuffer(GL_ARRAY_BUFFER, vboTBN);
		glBufferData(GL_ARRAY_BUFFER, vboTBNBufferSize, 0, GL_STATIC_DRAW);
		vboTBNEnd = 0;
#endif // SHOW_TANGENT_SPACE
		InitMesh();

		LOGI("VBO: %03.1f%%", static_cast<float>(vboEnd * 100) / static_cast<float>(vboBufferSize));
		LOGI("IBO: %03.1f%%", static_cast<float>(iboEnd * 100) / static_cast<float>(iboBufferSize));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	{
		const FBOInfo fboMainInfo = GetFBOInfo(FBO_Main);
		const auto& tex = *textureList[fboMainInfo.name];
		glGenFramebuffers(1, fboMainInfo.p);
		glGenRenderbuffers(1, &depth);
		glBindRenderbuffer(GL_RENDERBUFFER, depth);
		glRenderbufferStorage(GL_RENDERBUFFER, depthComponentType, tex.Width(), tex.Height());
		glBindFramebuffer(GL_FRAMEBUFFER, *fboMainInfo.p);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex.TextureId());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.TextureId(), 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOGE("Error: FrameBufferObject(%s) is not complete!\n", fboMainInfo.name);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteRenderbuffers(1, &depth);
			glDeleteFramebuffers(1, fboMainInfo.p);
			glBindTexture(GL_TEXTURE_2D, 0);
			*fboMainInfo.p = 0;
			depth = 0;
		}
	}

	for (int i = FBO_Sub; i < FBO_End; ++i) {
		const FBOInfo e = GetFBOInfo(i);
		const auto& tex = *textureList[e.name];
		glGenFramebuffers(1, e.p);
		glBindFramebuffer(GL_FRAMEBUFFER, *e.p);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex.TextureId());
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.TextureId(), 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOGE("Error: FrameBufferObject(%s) is not complete!\n", e.name);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, e.p);
			glBindTexture(GL_TEXTURE_2D, 0);
			*e.p = 0;
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#ifdef __ANDROID__
#else
	eglSwapInterval(display, 1);
#endif // __ANDROID__
	isInitialized = true;
}

void Renderer::DrawFont(const Position2F& pos, const char* str)
{
  const Shader& shader = shaderList["default2D"];
  glUseProgram(shader.program);
  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_CULL_FACE);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

  static const int32_t stride = sizeof(Vertex);
  static const void* const offPosition = reinterpret_cast<void*>(offsetof(Vertex, position));
  static const void* const offTexCoord = reinterpret_cast<void*>(offsetof(Vertex, texCoord[0]));
  for (int i = 0; i < VertexAttribLocation_Max; ++i) {
	glDisableVertexAttribArray(i);
  }
  glEnableVertexAttribArray(VertexAttribLocation_Position);
  glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
  glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
  glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord);

  const Matrix4x4 mP = { {
	  2.0f / viewport[2], 0,                  0,                                    0,
	  0,                  -2.0f / viewport[3], 0,                                    0,
	0,                  0,                  -2.0f / (500.0f - 0.1f),              0,
	-1.0f,              1.0f,              -((500.0f + 0.1f) / (500.0f - 0.1f)), 1,
	} };
  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mP.f);
  Matrix4x4 mV = LookAt(Position3F(0, 0, 10), Position3F(0, 0, 0), Vector3F(0, 1, 0));
  glUniform1i(shader.texDiffuse, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureList["ascii"]->TextureId());
  glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 0.0f, 0.0f);
  const Mesh::Mesh& mesh = meshList["ascii"];
  float x = pos.x;
  for (const char* p = str; *p; ++p) {
	Matrix4x4 mMV = Matrix4x4::FromScale(0.5f, 0.5f, 1.0f);
	mMV = mV * Matrix4x4::Translation(x, pos.y, 0) * mMV;
	glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mMV.f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.materialList[0].iboOffset + *p * 6 * sizeof(GLushort)));
	x += 8.0f;
  }
  glEnable(GL_CULL_FACE);
}

/** Add Font string.

  @param x      x position of start of text. 0.0 is left, 1.0 is right.
  @param y      y position of start of text. 0.0 is top. 1.0 is bottom.
  @param scale  rendering scale. 1.0 is actual font size.
  @param color  rendering color.
  @param str    pointer to rendering string.
  @param options reidering options. @sa FontOption
  @param uw     width par character. if it is zero, each character width will be calculated automatically.
*/
void Renderer::AddString(float x, float y, float scale, const Color4B& color, const char* str, int options, float uw) {
  const size_t freeCount = MAX_FONT_RENDERING_COUNT - vboFontEnd / (sizeof(FontVertex) * 4);
  if (strlen(str) > freeCount) {
	LOGI("buffer over in AddString: %s", str);
	return;
  }
  std::vector<FontVertex> vertecies;
  Position2F curPos = Position2F(x, y) * Vector2F(2, -2) + Vector2F(-1, 1);
  while (const int c = *reinterpret_cast<const uint8_t*>(str++)) {
	const FontInfo& info = GetAsciiFontInfo(c);
	const float fw = info.GetWidth() * viewport[2] / referenceViewportSize.x;
	const float fh = info.GetHeight() * viewport[3] / referenceViewportSize.y;
	const float w = (uw == 0.0f ? (fw * (2.0f / viewport[2])) : uw) * scale;
	const float h = fh * (-2.0f / viewport[3]) * scale;
	vertecies.push_back({ curPos, info.leftTop, color });
	vertecies.push_back({ Position2F(curPos.x, curPos.y + h), Position2S(info.leftTop.x, info.rightBottom.y), color });
	vertecies.push_back({ Position2F(curPos.x + w, curPos.y), Position2S(info.rightBottom.x, info.leftTop.y), color });
	vertecies.push_back({ Position2F(curPos.x + w, curPos.y + h), info.rightBottom, color });
	curPos.x += w;
  }
  if (!vertecies.empty()) {
	glBindBuffer(GL_ARRAY_BUFFER, vboFont[currentFontBufferNo]);
	glBufferSubData(GL_ARRAY_BUFFER, vboFontEnd, vertecies.size() * sizeof(FontVertex), &vertecies[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	fontRenderingInfoList.push_back({ static_cast<GLint>(vboFontEnd / sizeof(FontVertex)), static_cast<GLsizei>(vertecies.size()), options });
	vboFontEnd += vertecies.size() * sizeof(FontVertex);
  }
}

/** Get string width on screen.
*/
float Renderer::GetStringWidth(const char* str) const {
  float w = 0.0f;
  const float d = 1.0f / referenceViewportSize.x;
  while (const char c = *(str++)) {
	const FontInfo& info = GetAsciiFontInfo(c);
	w += info.GetWidth() * d;
  }
  return w;
}

/** Get string height on screen.
*/
float Renderer::GetStringHeight(const char* str) const {
  float h = 0.0f;
  const float d = 1.0f / referenceViewportSize.y;
  while (const char c = *(str++)) {
	const FontInfo& info = GetAsciiFontInfo(c);
	h = std::max(h, info.GetHeight() * d);
  }
  return h;
}

/** Render font string that was added by AddString().
*/
void Renderer::DrawFontFoo()
{
  const Shader& shader = shaderList["font"];
  glUseProgram(shader.program);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);

  const Texture::TexturePtr fontTexture = textureList["font"];
  glUniform1i(shader.texDiffuse, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, fontTexture->TextureId());

  glBindBuffer(GL_ARRAY_BUFFER, vboFont[currentFontBufferNo]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  for (int i = 0; i < VertexAttribLocation_Max; ++i) {
	glDisableVertexAttribArray(i);
  }
  glEnableVertexAttribArray(VertexAttribLocation_Position);
  glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
  glEnableVertexAttribArray(VertexAttribLocation_Color);
  static const int32_t stride = sizeof(FontVertex);
  static const void* const offPosition = reinterpret_cast<void*>(offsetof(FontVertex, position));
  static const void* const offTexCoord = reinterpret_cast<void*>(offsetof(FontVertex, texCoord));
  static const void* const offColor = reinterpret_cast<void*>(offsetof(FontVertex, color));
  glVertexAttribPointer(VertexAttribLocation_Position, 2, GL_FLOAT, GL_FALSE, stride, offPosition);
  glVertexAttribPointer(VertexAttribLocation_TexCoord01, 2, GL_UNSIGNED_SHORT, GL_TRUE, stride, offTexCoord);
  glVertexAttribPointer(VertexAttribLocation_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offColor);
  const float tw = static_cast<float>(fontTexture->Width());
  const float th = static_cast<float>(fontTexture->Height());
  for (auto e : fontRenderingInfoList) {
	if (e.options & FONTOPTION_OUTLINE) {
	  glUniform4f(shader.fontOutlineInfo, 0.45f, 0.5f, 0.75f, 0.8f);
	} else if (e.options & FONTOPTION_KEEPCOLOR) {
	  glUniform4f(shader.fontOutlineInfo, 0.3f, 1.0f, -1.0f, 1.0f);
	} else {
	  glUniform4f(shader.fontOutlineInfo, 0.6f, 0.7f, -1.0f, 1.0f);
	}
	if (e.options & FONTOPTION_DROPSHADOW) {
	  if (e.options & FONTOPTION_OUTLINE) {
		glUniform4f(shader.fontDropShadowInfo, -3.0f / tw, 3.0f / th, 0.45f, 0.55f);
	  } else {
		glUniform4f(shader.fontDropShadowInfo, -3.0f / tw, 3.0f / th, 0.6f, 0.7f);
	  }
	} else {
	  glUniform4f(shader.fontDropShadowInfo, 0.0f, 0.0f, 0.6f, 0.7f);
	}
	glDrawArrays(GL_TRIANGLE_STRIP, e.first, e.count);
  }
  for (int i = 0; i < VertexAttribLocation_Max; ++i) {
	glDisableVertexAttribArray(i);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glEnable(GL_CULL_FACE);
}

void Renderer::Render(const ObjectPtr* begin, const ObjectPtr* end)
{
	static const int32_t stride = sizeof(Vertex);
	static const void* const offPosition = reinterpret_cast<void*>(offsetof(Vertex, position));
	static const void* const offWeight = reinterpret_cast<void*>(offsetof(Vertex, weight[0]));
	static const void* const offNormal = reinterpret_cast<void*>(offsetof(Vertex, normal));
	static const void* const offTangent = reinterpret_cast<void*>(offsetof(Vertex, tangent));
	static const void* const offBoneID = reinterpret_cast<void*>(offsetof(Vertex, boneID[0]));
	static const void* const offTexCoord01 = reinterpret_cast<void*>(offsetof(Vertex, texCoord[0]));

	if (!isInitialized) {
	  return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	// とりあえず適当なカメラデータからビュー行列を設定.
	const Position3F eye = cameraPos;
	const Position3F at = cameraPos + cameraDir;
	const Matrix4x4 mView = LookAt(eye, at, cameraUp);

	// 9:16のときを60をとし、アスペクト比に応じて変更する。
	static const float baseAspectRatio = 9.0f / 16.0f;
	const float aspectRatio = static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
	const float fov = 60.0f / baseAspectRatio * aspectRatio;

	// パフォーマンス計測準備.
	static const int FENCE_ID_SHADOW_PATH = 0;
	static const int FENCE_ID_SHADOW_FILTER_PATH = 1;
	static const int FENCE_ID_COLOR_PATH = 2;
	static const int FENCE_ID_HDR_PATH = 3;
	static const int FENCE_ID_FINAL_PATH = 4;
	GLuint fences[5];
	Local::glGenFencesNV(5, fences);

	// shadow path.
	const Vector3F shadowUp = (Dot(shadowLightDir, Vector3F(0, 1, 0)) > 0.99f) ? Vector3F(0, 0, -1) : Vector3F(0, 1, 0);
	const Matrix4x4 mViewL = LookAt(shadowLightPos, shadowLightPos + shadowLightDir, shadowUp);
	const Matrix4x4 mProjL = Olthographic(GetFBOInfo(FBO_Shadow).width, GetFBOInfo(FBO_Shadow).height, shadowNear, shadowFar);
	Matrix4x4 mCropL;
	{
	  // sqrt(480*480+800*800)/480=1.94365063
	  static const float slant = 1.94365063f;
	  static const float n = 0.1f;
	  static const float f = 20.0f;
	  const float t = tan(degreeToRadian<float>(fov * 0.5f * slant));
	  const Position2F a(n * t, n), c(f * t, f);
	  const Vector2F ca = c - a;
	  const Position2F p = a + ca * 0.5f;
	  const Vector2F v = Vector2F(-ca.y, ca.x).Normalize();
	  const float distance = p.y + p.x * v.y; // 視点から視錐台の外接円の中心までの距離.
	  //const float r = (a - Position2F(0, distance)).Length(); // 視錐台の外接円の半径.

	  const Vector3F vEye = (at - eye).Normalize();
	  const Position3F frustumCenter = eye + (vEye * distance);
	  Vector4F transformedCenter = mProjL * mViewL * Vector3F(frustumCenter.x, frustumCenter.y, frustumCenter.z);
	  //static float ms = 4.0f;// transformedCenter.w / frustumRadius;
	  const Matrix4x4 m = { {
	    shadowScale.x, 0, 0, 0,
	    0, shadowScale.y, 0, 0,
	    0, 0, 1, 0,
		0, 0, 0, 1,
	  } };
	  mCropL = m;
	}
	const Matrix4x4 mVPForShadow = mCropL * mProjL * mViewL;

#if 1
	{
		glEnableVertexAttribArray(VertexAttribLocation_Position);
		glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
		glEnableVertexAttribArray(VertexAttribLocation_Normal);
		glVertexAttribPointer(VertexAttribLocation_Normal, 3, GL_FLOAT, GL_FALSE, stride, offNormal);
		glDisableVertexAttribArray(VertexAttribLocation_Tangent);
#ifdef USE_ALPHA_TEST_IN_SHADOW_RENDERING
		glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
		glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);
#else
		glDisableVertexAttribArray(VertexAttribLocation_TexCoord01);
#endif // USE_ALPHA_TEST_IN_SHADOW_RENDERING
		glEnableVertexAttribArray(VertexAttribLocation_Weight);
		glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
		glEnableVertexAttribArray(VertexAttribLocation_BoneID);
		glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);

		const FBOInfo fboShadowInfo = GetFBOInfo(FBO_Shadow);
		glBindFramebuffer(GL_FRAMEBUFFER, *fboShadowInfo.p);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_ONE, GL_ZERO);
		glCullFace(GL_BACK);

		glViewport(0, 0, fboShadowInfo.width, fboShadowInfo.height);
		glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const Shader& shader = shaderList["shadow"];
		glUseProgram(shader.program);
		glUniform3f(shader.lightDirForShadow, shadowLightDir.x, shadowLightDir.y, shadowLightDir.z);
		glUniformMatrix4fv(shader.matLightForShadow, 1, GL_FALSE, mVPForShadow.f);

		for (const ObjectPtr* itr = begin; itr != end; ++itr) {
			const Object& obj = *itr->get();
			if (!obj.IsValid() || obj.shadowCapability == ShadowCapability::Disable) {
				continue;
			}

#ifdef USE_ALPHA_TEST_IN_SHADOW_RENDERING
			{
			  const Mesh::Mesh& mesh = *obj.GetMesh();
			  glActiveTexture(GL_TEXTURE0);
			  if (mesh.texDiffuse) {
				glBindTexture(GL_TEXTURE_2D, mesh.texDiffuse->TextureId());
			  } else {
				glBindTexture(GL_TEXTURE_2D, 0);
			  }
			}
#endif // USE_ALPHA_TEST_IN_SHADOW_RENDERING

			const size_t boneCount = std::min(obj.GetBoneCount(), static_cast<size_t>(32));
			if (boneCount) {
				glUniform4fv(shader.bones, boneCount * 3, obj.GetBoneMatirxArray());
			} else {
			  Matrix4x3 mScale = Matrix4x3::Unit();
			  mScale.Set(0, 0, obj.Scale().x);
			  mScale.Set(1, 1, obj.Scale().y);
			  mScale.Set(2, 2, obj.Scale().z);
			  const Matrix4x3 m =  ToMatrix(obj.RotTrans()) * mScale;
			  glUniform4fv(shader.bones, 3, m.f);
			}
			obj.GetMesh()->Draw();
		}
		if(0){
		  glCullFace(GL_BACK);
		  // sqrt(480*480+800*800)/480=1.94365063
		  static const float slant = 1.94365063f;
		  static const float n = 0.1f;
		  static const float f = 20.0f;
		  const float t = tan(degreeToRadian<float>(fov * 0.5f));
		  const Position2F a(n * t * slant, n), c(f * t * slant, f);
		  const Vector2F ca = c - a;
		  const Position2F p = a + ca * 0.5f;
		  const Vector2F v = Vector2F(-ca.y, ca.x).Normalize();
		  const float distance = p.y + p.x * v.y; // 視点から視錐台の外接円の中心までの距離.
		  const float r = (a - Position2F(0, distance)).Length(); // 視錐台の外接円の半径.

		  const Vector3F vEye = (at - eye).Normalize();
		  const Position3F frustumCenter = eye + (vEye * distance);
		  Matrix4x4 m = Matrix4x4::FromScale(r * 2, r * 2, r * 2);
		  m.Set(0, 3, frustumCenter.x);
		  m.Set(1, 3, frustumCenter.y);
		  m.Set(2, 3, frustumCenter.z);
		  glUniform4fv(shader.bones, 3, m.f);
		  meshList["Sphere"].Draw();
		}

		Local::glSetFenceNV(fences[FENCE_ID_SHADOW_PATH], GL_ALL_COMPLETED_NV);
	}
#if 1
	// fboMain ->(bilinear4x4)-> fboShadow1
	{
		glEnableVertexAttribArray(VertexAttribLocation_Position);
		glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
		glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
		glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);

		const FBOInfo fboShadow1Info = GetFBOInfo(FBO_Shadow1);
		glBindFramebuffer(GL_FRAMEBUFFER, *fboShadow1Info.p);
		glViewport(0, 0, fboShadow1Info.width, fboShadow1Info.height);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE, GL_ZERO);
		glCullFace(GL_NONE);

		const Shader& shader = shaderList["bilinear4x4"];
		glUseProgram(shader.program);

		static float scaleY = 1.0f;
		Matrix4x4 mtx = Matrix4x4::Unit();
		mtx.Scale(1.0f, scaleY, 1.0f);
		glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);
		glUniformMatrix4fv(shader.matView, 1, GL_FALSE, Matrix4x4::Unit().f);

		glUniform1i(shader.texShadow, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureList["fboMain"]->TextureId());

		meshList["board2D"].Draw();
	}
#endif
#endif
	Local::glSetFenceNV(fences[FENCE_ID_SHADOW_FILTER_PATH], GL_ALL_COMPLETED_NV);

	// color path.
	const FBOInfo fboMainInfo = GetFBOInfo(FBO_Main);
	glBindFramebuffer(GL_FRAMEBUFFER, *fboMainInfo.p);
	glViewport(0, 0, fboMainInfo.width, fboMainInfo.height);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);

	glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnableVertexAttribArray(VertexAttribLocation_Position);
	glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
	glEnableVertexAttribArray(VertexAttribLocation_Normal);
	glVertexAttribPointer(VertexAttribLocation_Normal, 3, GL_FLOAT, GL_FALSE, stride, offNormal);
	glEnableVertexAttribArray(VertexAttribLocation_Tangent);
	glVertexAttribPointer(VertexAttribLocation_Tangent, 4, GL_FLOAT, GL_FALSE, stride, offTangent);
	glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
	glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);
	glEnableVertexAttribArray(VertexAttribLocation_Weight);
	glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
	glEnableVertexAttribArray(VertexAttribLocation_BoneID);
	glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);

	const Matrix4x4 mProj = Perspective(
	  fov,
	  static_cast<float>(viewport[2]),
	  static_cast<float>(viewport[3]),
	  1.0f,
	  5000.0f
	);

	static const struct {
	  float range;
	  float inverse;
	  Vector3F cloudColorMain;
	  Vector3F cloudColorEdge;
	} iblDynamicRangeArray[] = {
	  { 0.5f, 1.0f / 0.5f, Vector3F(0.65f, 0.75f, 0.85f), Vector3F(0.0f, 0.05f, 0.2f) },
	  { 0.5f, 1.0f / 0.5f, Vector3F(0.65f, 0.5f, 0.3f), Vector3F(0.75f, 0.1f, 0.05f) },
	  { 0.5f, 1.0f / 2.0f, Vector3F(1.5f, 1.5f, 1.2f), Vector3F(0.4f, 0.1f, 0.5f) },
	};
	const float dynamicRangeFactor = iblDynamicRangeArray[timeOfScene].range;

	// 適当にライトを置いてみる.
	const Vector4F lightPos(50, 50, 50, 1.0);
	const Vector3F lightColor(7000.0f, 6000.0f, 5000.0f);
#if 1
	if (doesDrawSkybox && hasIBLTextures) {
	  const Shader& shader = shaderList["skybox"];
	  glUseProgram(shader.program);

	  glUniform1f(shader.dynamicRangeFactor, dynamicRangeFactor);

	  Matrix4x4 mTrans(Matrix4x4::Unit());
	  mTrans.SetVector(3, Vector4F(eye.x, eye.y, eye.z, 1));
	  const Matrix4x4 m = mView * mTrans;
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
	  glUniformMatrix4fv(shader.matView, 1, GL_FALSE, m.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_CUBE_MAP, iblSpecularSourceList[0]->TextureId());
	  glActiveTexture(GL_TEXTURE1);
	  glBindTexture(GL_TEXTURE_2D, 0);
	  glActiveTexture(GL_TEXTURE2);
	  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	  glActiveTexture(GL_TEXTURE3);
	  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	  glActiveTexture(GL_TEXTURE4);
	  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	  glActiveTexture(GL_TEXTURE5);
	  glBindTexture(GL_TEXTURE_2D, 0);
	  meshList["skybox"].Draw();
	  Local::glSetFenceNV(fences[FENCE_ID_COLOR_PATH], GL_ALL_COMPLETED_NV);
	}

	const GLuint cloudProgramId = shaderList["cloud"].program;
	const GLuint seaProgramId = shaderList["sea"].program;
	GLuint currentProgramId = 0;
	const int iblSourceSize = iblSpecularSourceList.size() - 1;
	for (const ObjectPtr* itr = begin; itr != end; ++itr) {
		const Object& obj = *itr->get();
		if (!obj.IsValid() || obj.shadowCapability == ShadowCapability::ShadowOnly || !hasIBLTextures) {
			continue;
		}

		const Shader& shader = *obj.GetShader();
		if (shader.program && shader.program != currentProgramId) {
			glUseProgram(shader.program);
			currentProgramId = shader.program;

			glUniform1f(shader.dynamicRangeFactor, dynamicRangeFactor);

			glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
			glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mView.f);
			glUniformMatrix4fv(shader.matLightForShadow, 1, GL_FALSE, mVPForShadow.f);

			glUniform3f(shader.eyePos, eye.x, eye.y, eye.z);
			glUniform3f(shader.lightColor, lightColor.x, lightColor.y, lightColor.z);
			glUniform3f(shader.lightPos, lightPos.x, lightPos.y, lightPos.z);

			glUniform1i(shader.texDiffuse, 0);
			glUniform1i(shader.texNormal, 1);
			static const int texIBLId[] = { 2, 3, 4 };
			glUniform1iv(shader.texIBL, 3, texIBLId);
			glUniform1i(shader.texShadow, 5);


			if (shader.program == cloudProgramId) {
				for (int i = 0; i < 4; ++i) {
					glActiveTexture(GL_TEXTURE2 + i);
					glBindTexture(GL_TEXTURE_2D, 0);
				}
				glDepthMask(GL_FALSE);
				glDisable(GL_CULL_FACE);
			} else {
				// IBL用テクスチャを設定.
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_CUBE_MAP, iblSpecularSourceList[0]->TextureId());
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_CUBE_MAP, iblSpecularSourceList[3]->TextureId());
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_CUBE_MAP, iblDiffuseSourceList->TextureId());
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, textureList["fboShadow1"]->TextureId());
				glDepthMask(GL_TRUE);
				glEnable(GL_CULL_FACE);
			}
#ifdef SUNNYSIDEUP_DEBUG
			static float debug = -1;
			glUniform1f(shader.debug, debug);
#endif // SUNNYSIDEUP_DEBUG
		}

		const Vector4F materialColor = obj.Color().ToVector4F();
		if (shader.program == cloudProgramId) {
		  const auto& e = iblDynamicRangeArray[timeOfScene];
		  const Vector3F color0 = e.cloudColorMain * e.range * e.inverse;
		  const Vector3F color1 = e.cloudColorEdge * e.range * e.inverse;
		  glUniform4f(shader.materialColor, color0.x, color0.y, color0.z, materialColor.w);
		  glUniform3f(shader.cloudColor, color1.x, color1.y, color1.z);
		} else {
		  glUniform4fv(shader.materialColor, 1, &materialColor.x);
		}
		const float metallic = obj.Metallic();
		const float roughness = obj.Roughness();
		if (shader.program == seaProgramId) {
		  glUniform3f(shader.materialMetallicAndRoughness, metallic, roughness, animationTick);
		} else {
		  glUniform2f(shader.materialMetallicAndRoughness, metallic, roughness);
		}

		const Mesh::Mesh& mesh = *obj.GetMesh();
		{
			glActiveTexture(GL_TEXTURE0);
			if (mesh.texDiffuse) {
				glBindTexture(GL_TEXTURE_2D, mesh.texDiffuse->TextureId());
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			glActiveTexture(GL_TEXTURE1);
			if (mesh.texNormal) {
				glBindTexture(GL_TEXTURE_2D, mesh.texNormal->TextureId());
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}

		const size_t boneCount = std::min(obj.GetBoneCount(), static_cast<size_t>(32));
		if (boneCount) {
			glUniform4fv(shader.bones, boneCount * 3, obj.GetBoneMatirxArray());
		} else {
		  Matrix4x3 mScale = Matrix4x3::Unit();
		  mScale.Set(0, 0, obj.Scale().x);
		  mScale.Set(1, 1, obj.Scale().y);
		  mScale.Set(2, 2, obj.Scale().z);
		  const Matrix4x3 m = ToMatrix(obj.RotTrans()) * mScale;
		  glUniform4fv(shader.bones, 3, m.f);
		  if (shader.type == ShaderType::Simple3D) {
			Matrix4x4 mm;
			mm.SetVector(0, Vector4F(m.f[0], m.f[1], m.f[2], 0.0f));
			mm.SetVector(1, Vector4F(m.f[4], m.f[5], m.f[6], 0.0f));
			mm.SetVector(2, Vector4F(m.f[8], m.f[9], m.f[10], 0.0f));
			mm.SetVector(3, Vector4F(m.f[3], m.f[7], m.f[11], 1.0f));
			mm.Inverse();
			const Vector4F invEye = mm * Vector4F(eye.x, eye.y, eye.z, 1.0f);
			glUniform3f(shader.eyePos, invEye.x, invEye.y, invEye.z);
		  }
		}
		glActiveTexture(GL_TEXTURE2);
		for (auto& e : mesh.materialList) {
			const float m = std::min(1.0f, std::max(0.0f, e.material.metallic.To<float>() - metallic));
			const float r = std::min(1.0f, std::max(0.0f, e.material.roughness.To<float>() + roughness));
			const int index = std::min(iblSourceSize, std::max(0, static_cast<int>(r * static_cast<float>(iblSourceSize) + 0.5f)));
			glBindTexture(GL_TEXTURE_CUBE_MAP, iblSpecularSourceList[index]->TextureId());
			if (shader.program == seaProgramId) {
			  glUniform3f(shader.materialMetallicAndRoughness, m, r, m > 0.5f ? animationTick * 0.25f : 0.0f);
			} else {
			  glUniform2f(shader.materialMetallicAndRoughness, m, r);
			}
			glDrawElements(GL_TRIANGLES, e.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(e.iboOffset));
		}
	}
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);

#ifdef SHOW_TANGENT_SPACE
	static bool showTangentSpace = true;
	if (showTangentSpace) {
	  static const size_t stride = sizeof(TBNVertex);
	  static const void* const offPosition = reinterpret_cast<void*>(offsetof(TBNVertex, position));
	  static const void* const offColor = reinterpret_cast<void*>(offsetof(TBNVertex, color));
	  static const void* const offWeight = reinterpret_cast<void*>(offsetof(TBNVertex, weight));
	  static const void* const offBoneID = reinterpret_cast<void*>(offsetof(TBNVertex, boneID));

	  glBindBuffer(GL_ARRAY_BUFFER, vboTBN);
	  glEnableVertexAttribArray(VertexAttribLocation_Position);
	  glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
	  glEnableVertexAttribArray(VertexAttribLocation_Weight);
	  glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
	  glEnableVertexAttribArray(VertexAttribLocation_BoneID);
	  glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);
	  glEnableVertexAttribArray(VertexAttribLocation_Color);
	  glVertexAttribPointer(VertexAttribLocation_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, offColor);
	  const Shader& shader = shaderList["tbn"];
	  glUseProgram(shader.program);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
	  glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mView.f);
	  for (const ObjectPtr* itr = begin; itr != end; ++itr) {
		const Object& obj = *itr->get();
		if (!obj.IsValid()) {
		  continue;
		}
		const size_t boneCount = std::min(obj.GetBoneCount(), static_cast<size_t>(32));
		if (boneCount) {
		  glUniform4fv(shader.bones, boneCount * 3, obj.GetBoneMatirxArray());
		} else {
		  Matrix4x3 mScale = Matrix4x3::Unit();
		  mScale.Set(0, 0, obj.Scale().x);
		  mScale.Set(1, 1, obj.Scale().y);
		  mScale.Set(2, 2, obj.Scale().z);
		  const Matrix4x3 m = ToMatrix(obj.RotTrans()) * mScale;
		  glUniform4fv(shader.bones, 3, m.f);
		}
		const Mesh::Mesh& mesh = *obj.GetMesh();
		if (mesh.vboTBNCount) {
		  glDrawArrays(GL_LINES, mesh.vboTBNOffset, mesh.vboTBNCount);
		}
	  }
	}
	glDisableVertexAttribArray(VertexAttribLocation_Color);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
#endif // SHOW_TANGENT_SPACE
#endif

#ifdef USE_HDR_BLOOM
	// hdr path.
	glEnableVertexAttribArray(VertexAttribLocation_Position);
	glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
	glEnableVertexAttribArray(VertexAttribLocation_Normal);
	glVertexAttribPointer(VertexAttribLocation_Normal, 3, GL_FLOAT, GL_FALSE, stride, offNormal);
	glEnableVertexAttribArray(VertexAttribLocation_Tangent);
	glVertexAttribPointer(VertexAttribLocation_Tangent, 4, GL_FLOAT, GL_FALSE, stride, offTangent);
	glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
	glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);
	glEnableVertexAttribArray(VertexAttribLocation_Weight);
	glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
	glEnableVertexAttribArray(VertexAttribLocation_BoneID);
	glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);

	// fboMain ->(reduceLum)-> fboSub
	{
	  const FBOInfo fboSubInfo = GetFBOInfo(FBO_Sub);
	  glBindFramebuffer(GL_FRAMEBUFFER, *fboSubInfo.p);
	  glViewport(0, 0, fboSubInfo.width, fboSubInfo.height);
	  glDisable(GL_DEPTH_TEST);
	  glBlendFunc(GL_ONE, GL_ZERO);
	  glCullFace(GL_BACK);

	  const Shader& shader = shaderList["reduceLum"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboMain"]->TextureId());
	  meshList["board2D"].Draw();
	}
	// fboSub ->(hdrdiff)-> fboHDR[1]
	{
	  const FBOInfo fboHDR1Info = GetFBOInfo(FBO_HDR1);
	  glBindFramebuffer(GL_FRAMEBUFFER, *fboHDR1Info.p);
	  glViewport(0, 0, fboHDR1Info.width, fboHDR1Info.height);

	  const Shader& shader = shaderList["hdrdiff"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);
	  glUniform2f(shader.dynamicRangeFactor, iblDynamicRangeArray[timeOfScene].range, 1.0f / (1.0f - iblDynamicRangeArray[timeOfScene].range));

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboSub"]->TextureId());
	  meshList["board2D"].Draw();
	}

	static const bool useWideBloom = false;

	// fboHDR[1] ->(sample4)-> fboHDR[0] ... fboHDR[4]
	{
	  const Shader& shader = shaderList["sample4"];
	  glUseProgram(shader.program);
	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);
	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  const Mesh::Mesh& mesh = meshList["board2D"];

	  if (useWideBloom) {
		for (int i = FBO_HDR1; ;) {
		  const FBOInfo fboInfoDest = GetFBOInfo(i - 1);
		  const FBOInfo fboInfo = GetFBOInfo(i);
		  {
			glBindFramebuffer(GL_FRAMEBUFFER, *fboInfoDest.p);
			glViewport(0, 0, fboInfo.width, fboInfo.height);

			glBindTexture(GL_TEXTURE_2D, textureList[fboInfo.name]->TextureId());
			glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 0.5f / fboInfo.width, 0.5f / fboInfo.height);
			mesh.Draw();
		  }
		  if (i >= FBO_HDR5) {
			break;
		  }
		  ++i;
		  {
			const FBOInfo fboInfoNext = GetFBOInfo(i);
			glBindFramebuffer(GL_FRAMEBUFFER, *fboInfoNext.p);
			glViewport(0, 0, fboInfoNext.width, fboInfoNext.height);

			glBindTexture(GL_TEXTURE_2D, textureList[fboInfoDest.name]->TextureId());
			glUniform4f(
			  shader.unitTexCoord,
			  static_cast<float>(fboInfo.width) / static_cast<float>(fboInfoDest.width),
			  static_cast<float>(fboInfo.height) / static_cast<float>(fboInfoDest.height),
			  0.25f / fboInfoDest.width,
			  0.25f / fboInfoDest.height
			);
			mesh.Draw();
		  }
		}
	  } else {
		for (int i = FBO_HDR1; i < FBO_HDR5; ++i) {
		  const FBOInfo fboInfoDest = GetFBOInfo(i + 1);
		  glBindFramebuffer(GL_FRAMEBUFFER, *fboInfoDest.p);
		  glViewport(0, 0, fboInfoDest.width, fboInfoDest.height);

		  const FBOInfo fboInfoSrc = GetFBOInfo(i);
		  glBindTexture(GL_TEXTURE_2D, textureList[fboInfoSrc.name]->TextureId());
		  glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 0.25f / fboInfoSrc.width, 0.25f / fboInfoSrc.height);
		  mesh.Draw();
		}
	  }
	}
	// fboHDR[4] ->(default2D)-> fboHDR[4] ... fboHDR[0]
	{
	  const Shader& shader = shaderList["default2D"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);
	  glUniformMatrix4fv(shader.matView, 1, GL_FALSE, Matrix4x4::Unit().f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);

	  glBlendFunc(GL_ONE, GL_ONE);
	  const Mesh::Mesh& mesh = meshList["board2D"];
	  if (useWideBloom) {
		for (int i = FBO_HDR4; i > FBO_HDR0; --i) {
		  const FBOInfo fboInfo = GetFBOInfo(i + 1);
		  const FBOInfo fboInfoSrc = GetFBOInfo(i);
		  const FBOInfo fboInfoDest = GetFBOInfo(i - 1);
		  glBindFramebuffer(GL_FRAMEBUFFER, *fboInfoDest.p);
		  glViewport(0, 0, fboInfoSrc.width, fboInfoSrc.height);
		  glBindTexture(GL_TEXTURE_2D, textureList[fboInfoSrc.name]->TextureId());
		  glUniform4f(shader.unitTexCoord, static_cast<float>(fboInfo.width) / static_cast<float>(fboInfoSrc.width), static_cast<float>(fboInfo.height) / static_cast<float>(fboInfoSrc.height), 0.0f, 0.0f);
		  mesh.Draw();
		}
	  } else {
		for (int i = FBO_HDR5; i > FBO_HDR1; --i) {
		  const FBOInfo fboInfoDest = GetFBOInfo(i - 1);
		  glBindFramebuffer(GL_FRAMEBUFFER, *fboInfoDest.p);
		  glViewport(0, 0, fboInfoDest.width, fboInfoDest.height);

		  const FBOInfo fboInfoSrc = GetFBOInfo(i);
		  glBindTexture(GL_TEXTURE_2D, textureList[fboInfoSrc.name]->TextureId());
		  glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 0.0f, 0.0f);
		  mesh.Draw();
		}
	  }
	}
#endif // USE_HDR_BLOOM
	Local::glSetFenceNV(fences[FENCE_ID_HDR_PATH], GL_ALL_COMPLETED_NV);

	// final path.
	{
	  glBindFramebuffer(GL_FRAMEBUFFER, 0);
	  glViewport(0, 0, width, height);
	  glDisable(GL_DEPTH_TEST);
	  glDepthFunc(GL_ALWAYS);
	  glCullFace(GL_BACK);

	  glEnableVertexAttribArray(VertexAttribLocation_Position);
	  glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
	  glDisableVertexAttribArray(VertexAttribLocation_Normal);
	  glDisableVertexAttribArray(VertexAttribLocation_Tangent);
	  glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
	  glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);
	  glDisableVertexAttribArray(VertexAttribLocation_Weight);
	  glDisableVertexAttribArray(VertexAttribLocation_BoneID);

	  const Shader& shader = shaderList["applyhdr"];
	  glUseProgram(shader.program);
	  glBlendFunc(GL_ONE, GL_ZERO);

	  glUniform1f(shader.dynamicRangeFactor, iblDynamicRangeArray[timeOfScene].inverse);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  const Vector4F color = filterColor.ToVector4F();
	  glUniform4f(shader.materialColor, color.x, color.y, color.z, color.w);

	  static const int texSource[] = { 0, 1, 2 };
	  glUniform1iv(shader.texSource, 3, texSource);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboMain"]->TextureId());
	  glActiveTexture(GL_TEXTURE1);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboSub"]->TextureId());

#ifdef USE_HDR_BLOOM
	  glActiveTexture(GL_TEXTURE2);
	  if (useWideBloom) {
		glBindTexture(GL_TEXTURE_2D, textureList["fboHDR0"]->TextureId());
	  } else {
		glBindTexture(GL_TEXTURE_2D, textureList["fboHDR1"]->TextureId());
	  }
#endif // USE_HDR_BLOOM

	  meshList["board2D"].Draw();

	  Local::glSetFenceNV(fences[FENCE_ID_FINAL_PATH], GL_ALL_COMPLETED_NV);
	}

	DrawFontFoo();

#ifndef NDEBUG
	// パフォーマンス計測.
	{
	  int64_t fenceTimes[6];
	  fenceTimes[0] = GetCurrentTime();
	  for (int i = 0; i < 5; ++i) {
		Local::glFinishFenceNV(fences[i]);
		fenceTimes[i + 1] = GetCurrentTime();
	  }
	  int64_t diffTimes[6];
	  for (int i = 0; i < 5; ++i) {
		diffTimes[i] = std::max<int64_t>(fenceTimes[i + 1] - fenceTimes[i], 0);
	  }
	  diffTimes[5] = std::accumulate(diffTimes, diffTimes + 5, static_cast<int64_t>(0));
	  static const char* const fenceNameList[] = {
		"SHADOW:",
		"FILTER:",
		"COLOR :",
		"HDR   :",
		"FINAL :",
		"TOTAL :",
	  };
	  static const float targetTime = 1000000000.0f / 30.0f;
	  for (int i = 0; i < 6; ++i) {
		std::string s(fenceNameList[i]);
		const int percentage = static_cast<int>((static_cast<float>(diffTimes[i]) * 1000.0f) / targetTime);// totalTime;
		s += '0' + percentage / 1000;
		s += '0' + (percentage % 1000) / 100;
		s += '0' + (percentage % 100) / 10;
		s += '.';
		s += '0' + percentage % 10;
//		s += boost::lexical_cast<std::string>(diffTimes[i]);
		DrawFont(Position2F(viewport[2] * 0.025f, static_cast<float>(viewport[3] - (16 * 8) + 16 * i)), s.c_str());
	  }
	  Local::glDeleteFencesNV(5, fences);
	}

	{
	  auto f = [this](float pos, char c, float value) {
		char buf[16] = { 0 };
		buf[0] = c; buf[1] = ':'; buf[2] = value >= 0.0f ? ' ' : '-';
		const int x = std::abs<int>(static_cast<int>(value * 100.0f));
		for (int i = 3, s = 100000; i < 10; ++i) {
		  if (i == 7) {
			buf[i] = '.';
		  } else {
			buf[i] = '0' + (x / s) % 10;
			s /= 10;
		  }
		}
		buf[10] = '\0';
		DrawFont(Position2F(392.0f, pos), buf);
	  };
	  f( 4, 'X', cameraPos.x);
	  f(20, 'Y', cameraPos.y);
	  f(36, 'Z', cameraPos.z);
	  f(52, 'x', cameraDir.x);
	  f(68, 'y', cameraDir.y);
	  f(84, 'z', cameraDir.z);

	  for (auto& e : debugStringList) {
		DrawFont(Position2F(e.pos.x, e.pos.y), e.str.c_str());
	  }
	}

#if 0
	{
	  const Shader& shader = shaderList["default2D"];
	  glUseProgram(shader.program);
	  glBlendFunc(GL_ONE, GL_ZERO);

	  int32_t viewport[4];
	  glGetIntegerv(GL_VIEWPORT, viewport);
	  const Matrix4x4 mP = { {
		  2.0f / viewport[2], 0,                  0,                                    0,
		  0,                  -2.0f / viewport[3], 0,                                    0,
		0,                  0,                  -2.0f / (500.0f - 0.1f),              0,
		-1.0f,              1.0f,              -((500.0f + 0.1f) / (500.0f - 0.1f)), 1,
		} };
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mP.f);
	  Matrix4x4 mV = LookAt(Position3F(0, 0, 10), Position3F(0, 0, 0), Vector3F(0, 1, 0));
	  Matrix4x4 mMV = Matrix4x4::Unit();
	  mMV.Scale(128.0f, 128.0f, 0.0f);
	  mMV = mV * Matrix4x4::Translation(128, 128 + 8 + 56, 0) * mMV;
	  glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mMV.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboShadow1"]->TextureId());
	  glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 0.0f, 0.0f);
	  meshList["board2D"].Draw();
	}
#endif
#endif // NDEBUG

	// テクスチャのバインドを解除.
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// バッファオブジェクトのバインドを解除.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::Update(float dTime, const Position3F& pos, const Vector3F& dir, const Vector3F& up)
{
  animationTick += dTime;

  cameraPos = pos;
  cameraDir = dir;
  cameraUp = up;

  if (filterMode != FILTERMODE_NONE) {
	filterTimer += dTime;
	if (filterTimer >= filterTargetTime) {
	  filterTimer = filterTargetTime;
	}
	if (filterMode == FILTERMODE_FADEOUT) {
	  filterColor.a = static_cast<GLubyte>(255.0f * (filterTimer / filterTargetTime));
	} else if (filterMode == FILTERMODE_FADEIN) {
	  filterColor.a = static_cast<GLubyte>(255.0f * (1.0f - filterTimer / filterTargetTime));
	}
	if (filterTimer >= filterTargetTime) {
	  filterMode = FILTERMODE_NONE;
	}
  }
}

void Renderer::Unload()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (int i = FBO_End - 1; i >= 0; --i) {
	  const FBOInfo& e = GetFBOInfo(i);
	  if (*e.p) {
		glDeleteRenderbuffers(1, e.p);
		*e.p = 0;
	  }
	}

	animationList.clear();
	meshList.clear();
	textureList.clear();

	for (auto itr = shaderList.rbegin(); itr != shaderList.rend(); ++itr) {
	  const int programId = itr->second.program;
	  glDeleteProgram(programId);
	}
	shaderList.clear();

	if (vbo) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (vboFont[0]) {
	  glDeleteBuffers(2, vboFont);
	  vboFont[0] = vboFont[1] = 0;
	}
	if (ibo) {
		glDeleteBuffers(1, &ibo);
		ibo = 0;
	}
#ifdef SHOW_TANGENT_SPACE
	if (vboTBN) {
	  glDeleteBuffers(1, &vboTBN);
	  vboTBN = 0;
	}
#endif // SHOW_TANGENT_SPACE

	if (display != EGL_NO_DISPLAY) {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (context != EGL_NO_CONTEXT) {
			eglDestroyContext(display, context);
		}
		if (surface != EGL_NO_SURFACE) {
			eglDestroySurface(display, surface);
		}
		eglTerminate(display);
	}
	display = EGL_NO_DISPLAY;
	context = EGL_NO_CONTEXT;
	surface = EGL_NO_SURFACE;

	isInitialized = false;
}

void Renderer::Swap()
{
	currentFontBufferNo ^= 1;
	vboFontEnd = 0;
	fontRenderingInfoList.clear();
	eglSwapBuffers(display, surface);
}

namespace {
	Vector3F MakeTangentVector(const Vector3F& normal) {
	  Vector3F c1 = normal.Cross(Vector3F(0.0f, 1.0f, 0.0f));
	  Vector3F c2 = normal.Cross(Vector3F(0.0f, 0.0f, 1.0f));
	  return (c1.LengthSq() >= c2.LengthSq() ? c1 : c2).Normalize();
	}

	Vertex CreateVertex(const Position3F& pos, const Vector3F& normal, const Position2F& texcoord) {
		Vertex v;
		v.position = pos;
		v.weight[0] = 255;
		v.weight[1] = v.weight[2] = v.weight[3] = 0;
		v.normal = normal;
		v.tangent = MakeTangentVector(normal);
		v.tangent.w = 1.0f;
		v.boneID[0] = v.boneID[1] = v.boneID[2] = v.boneID[3] = 0;
		v.texCoord[0] = Position2S::FromFloat(texcoord.x, texcoord.y);
		v.texCoord[1] = v.texCoord[0];
		return v;
	}
}

void Renderer::LoadFBX(const char* filename, const char* diffuse, const char* normal, bool showTBN)
{
  if (auto pBuf = FileSystem::LoadFile(filename)) {
#ifdef SHOW_TANGENT_SPACE
	Mesh::ImportMeshResult result = Mesh::ImportMesh(*pBuf, vbo, vboEnd, ibo, iboEnd, (showTBN ? vboTBN : 0), vboTBNEnd);
#else
	Mesh::ImportMeshResult result = Mesh::ImportMesh(*pBuf, vbo, vboEnd, ibo, iboEnd);
#endif // SHOW_TANGENT_SPACE
	if (result.result == Mesh::Result::success) {
	  for (auto m : result.meshes) {
		const auto itr = textureList.find(diffuse);
		if (itr != textureList.end()) {
		  m.texDiffuse = itr->second;
		}
		const auto itrNml = textureList.find(normal);
		if (itrNml != textureList.end()) {
		  m.texNormal = itrNml->second;
		}
		meshList.insert({ m.id, m });
	  }
	  for (auto e : result.animations) {
		animationList.insert({ e.id, e });
	  }
	} else {
	  static const char* const errorDescList[] = {
		"success",
		"invalidHeader",
		"noData",
		"invalidMeshInfo",
		"invalidIBO",
		"invalidVBO",
		"invalidJointInfo",
		"invalidAnimationInfo",
		"indexOverflow"
	  };
	  LOGE("ImportMesh fail by %s: '%s'", errorDescList[static_cast<int>(result.result)], filename);
	}
  }
}

/** 描画に必要なポリゴンメッシュの初期化.
* ポリゴンメッシュを読み込み、バッファオブジェクトに格納する.
* Initialize()から呼び出されるため、明示的に呼び出す必要はない.
*/
void Renderer::InitMesh()
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	LoadFBX("Meshes/sphere.msh", "Sphere", "Sphere_nml");
	LoadFBX("Meshes/flyingrock.msh", "flyingrock", "flyingrock_nml");
	LoadFBX("Meshes/brokenegg.msh", "brokenegg", "brokenegg_nml");
	LoadFBX("Meshes/accelerator.msh", "accelerator", "accelerator_nml");
	LoadFBX("Meshes/sunnysideup.msh", "sunnysideup", "sunnysideup_nml");
	LoadFBX("Meshes/SunnySideUp01.msh", "sunnysideup01", "sunnysideup01_nml");
	LoadFBX("Meshes/SunnySideUp02.msh", "sunnysideup02", "sunnysideup02_nml");
	LoadFBX("Meshes/SunnySideUp03.msh", "sunnysideup03", "sunnysideup03_nml");
	LoadFBX("Meshes/SunnySideUp04.msh", "sunnysideup04", "sunnysideup04_nml");
	LoadFBX("Meshes/OverMedium.msh", "overmedium", "overmedium_nml");
	LoadFBX("Meshes/block1.msh", "block1", "block1_nml");
	LoadFBX("Meshes/flyingpan.msh", "flyingpan", "flyingpan_nml");
	LoadFBX("Meshes/TargetCursor.msh", "dummy", "dummy");
	LoadFBX("Meshes/chickenegg.msh", "chickenegg", "chickenegg_nml");
	LoadFBX("Meshes/titlelogo.msh", "titlelogo", "titlelogo_nml");
	LoadFBX("Meshes/rock_collection.msh", "rock_s", "rock_s_nml");
	LoadFBX("Meshes/landscape.msh", "floor", "floor_nml");
	LoadFBX("Meshes/Coast.msh", "ls_coast", "ls_coast_nml");
	LoadFBX("Meshes/building00.msh", "building00", "building00_nml", true);
	LoadFBX("Meshes/building01.msh", "building01", "building01_nml", true);
	LoadFBX("Meshes/tower00.msh", "tower00", "tower00_nml", true);
	LoadFBX("Meshes/CoastTown.msh", "building01", "building01_nml");
	LoadFBX("Meshes/EggPack.msh", "EggPack", "EggPack_nml");
	LoadFBX("Meshes/CheckPoint.msh", "checkpoint", "checkpoint_nml");

	CreateSkyboxMesh();
	CreateUnitBoxMesh();
	CreateOctahedronMesh();
	CreateFloorMesh("ground", Vector3F(2000.0f, 2000.0f, 1.0f), 10);
	CreateBoardMesh("board2D", Vector3F(1.0f, 1.0f, 1.0f));
	CreateAsciiMesh("ascii");
	CreateCloudMesh("cloud0", Vector3F(100, 50, 100));
	CreateCloudMesh("cloud1", Vector3F(150, 50, 150));
	CreateCloudMesh("cloud2", Vector3F(150, 100, 150));
	CreateCloudMesh("cloud3", Vector3F(300, 100, 300));
}

void Renderer::CreateSkyboxMesh()
{
	std::vector<Vertex> vertecies;
	std::vector<GLushort> indices;
	vertecies.reserve(8);
	indices.reserve(2 * 6 * 3);
	static const GLfloat cubeVertices[] = {
		-1.0,  1.0,  1.0,
		-1.0, -1.0,  1.0,
		 1.0, -1.0,  1.0,
		 1.0,  1.0,  1.0,
		-1.0,  1.0, -1.0,
		-1.0, -1.0, -1.0,
		 1.0, -1.0, -1.0,
		 1.0,  1.0, -1.0,
	};
	for (int i = 0; i < 8 * 3; i += 3) {
		Vertex v;
		v.position = Position3F(cubeVertices[i + 0], cubeVertices[i + 1], cubeVertices[i + 2]) * (4000.0f / 1.7320508f);
		// this is only using position. so other parametar is not set.
		vertecies.push_back(v);
	}
	static const GLushort cubeIndices[] = {
		2, 1, 0, 0, 3, 2,
		6, 2, 3, 3, 7 ,6,
		5, 6, 7, 7, 4, 5,
		1, 5, 4, 4, 0, 1,
		7, 3, 0, 0, 4, 7,
//		1, 2, 6, 6, 5, 1,
	};
	const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
	for (auto e : cubeIndices) {
		indices.push_back(e + offset);
	}

	Mesh::Mesh mesh = Mesh::Mesh("skybox", iboEnd, indices.size());
	const auto itr = textureList.find("skybox_high");
	if (itr != textureList.end()) {
		mesh.texDiffuse = itr->second;
	}
	meshList.insert({ "skybox", mesh });

	glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
	vboEnd += vertecies.size() * sizeof(Vertex);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
	iboEnd += indices.size() * sizeof(GLushort);
}

void Renderer::CreateUnitBoxMesh()
{
  std::vector<Vertex> vertecies;
  std::vector<GLushort> indices;
  vertecies.reserve(3 * 2 * 6);
  indices.reserve(3 * 2 * 6);
  static const Position3F cubeVertices[] = {
	{ -1.0,  1.0,  1.0 },
	{ -1.0, -1.0,  1.0 },
	{ 1.0, -1.0,  1.0 },
	{ 1.0,  1.0,  1.0 },
	{ -1.0,  1.0, -1.0 },
	{ -1.0, -1.0, -1.0 },
	{ 1.0, -1.0, -1.0 },
	{ 1.0,  1.0, -1.0 },
  };
  static const GLushort cubeIndices[][3] = {
	{ 2, 1, 0 }, { 0, 3, 2 },
	{ 6, 2, 3 }, { 3, 7 ,6 },
	{ 5, 6, 7 }, { 7, 4, 5 },
	{ 1, 5, 4 }, { 4, 0, 1 },
	{ 7, 3, 0 }, { 0, 4, 7 },
	{ 1, 2, 6 }, { 6, 5, 1 },
  };
  static const Position2F texCoords[][3] = {
	{ { 0, 0 }, { 0, 1 }, { 1, 0 } },
	{ { 1, 0 }, { 1, 1 }, { 0, 1 } },
  };
  const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
  for (int i = 0; i < 2 * 6; ++i) {
	const Position3F& v0 = cubeVertices[cubeIndices[i][0]];
	const Position3F& v1 = cubeVertices[cubeIndices[i][1]];
	const Position3F& v2 = cubeVertices[cubeIndices[i][2]];
	const Vector3F normal = Normalize(Cross(v0 - v1, v2 - v1));
	const int index = i % 2;
	vertecies.push_back(CreateVertex(v0 * 0.5f, normal, texCoords[index][0]));
	vertecies.push_back(CreateVertex(v2 * 0.5f, normal, texCoords[index][1]));
	vertecies.push_back(CreateVertex(v1 * 0.5f, normal, texCoords[index][2]));
	indices.push_back(offset + i * 3 + 0);
	indices.push_back(offset + i * 3 + 1);
	indices.push_back(offset + i * 3 + 2);
  }

  Mesh::Mesh mesh = Mesh::Mesh("unitbox", iboEnd, indices.size());
  const auto itr = textureList.find("dummy");
  if (itr != textureList.end()) {
	mesh.texDiffuse = itr->second;
  }
  meshList.insert({ "unitbox", mesh });

  glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
  vboEnd += vertecies.size() * sizeof(Vertex);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
  iboEnd += indices.size() * sizeof(GLushort);
}

void Renderer::CreateOctahedronMesh()
{
  std::vector<Vertex> vertecies;
  std::vector<GLushort> indices;
  vertecies.reserve(8 * 3);
  indices.reserve(8 * 3 * 2);
  static const GLfloat positionList[][3] = {
	{ 0, 0, 1 },
	{ 0.125, 0, 0 },
	{ 0, 0.125, 0 },
	{-0.125, 0, 0 },
	{ 0, -0.125, 0 },
	{ 0, 0, -0.25 },
  };
  static const GLushort indexList[][3] = {
	{ 0, 1, 2 }, { 2, 1, 5 },
	{ 0, 2, 3 }, { 3, 2, 5 },
	{ 0, 3, 4 }, { 4, 3, 5 },
	{ 0, 4, 1 }, { 1, 4, 5 },
  };
  static const float texCoordList[][2] = { { 0, 0 }, { 0, 1 }, { 1, 1 } };
  const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
  int index = 0;
  for (auto& e : indexList) {
	const Position3F p[3] = {
	  Position3F(positionList[e[0]][0], positionList[e[0]][1], positionList[e[0]][2]),
	  Position3F(positionList[e[1]][0], positionList[e[1]][1], positionList[e[1]][2]),
	  Position3F(positionList[e[2]][0], positionList[e[2]][1], positionList[e[2]][2])
	};
	const Vector3F ab(p[1] - p[0]);
	const Vector3F ac(p[2] - p[0]);
	const Vector3F normal(ab.Cross(ac).Normalize());
	for (int i = 0; i < 3; ++i) {
	  vertecies.push_back(CreateVertex(p[i], normal, Position2F(texCoordList[i][0], texCoordList[i][1])));
	  indices.push_back(index + offset);
	  ++index;
	}
  }

  Mesh::Mesh mesh = Mesh::Mesh("octahedron", iboEnd, indices.size());
  const auto itr = textureList.find("dummy");
  if (itr != textureList.end()) {
	mesh.texDiffuse = itr->second;
  }
  meshList.insert({ "octahedron", mesh });

  glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
  vboEnd += vertecies.size() * sizeof(Vertex);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
  iboEnd += indices.size() * sizeof(GLushort);
}

void Renderer::CreateBoardMesh(const char* id, const Vector3F& scale)
{
	std::vector<Vertex> vertecies;
	std::vector<GLushort> indices;
	vertecies.reserve(4);
	indices.reserve(2 * 3);
	static const GLfloat cubeVertices[] = {
		-1,  1, 0,  0, 0,
		-1, -1, 0,  0, 1,
		 1, -1, 0,  1, 1,
		 1,  1, 0,  1, 0,
	};
	for (int i = 0; i < 4 * 5; i += 5) {
		vertecies.push_back(CreateVertex(
			Position3F(cubeVertices[i + 0], cubeVertices[i + 1], cubeVertices[i + 2]) * scale,
			Vector3F(0.0f, 0.0f, 1.0f),
			Position2F(cubeVertices[i + 3], cubeVertices[i + 4])
		));
	}
	static const GLushort cubeIndices[] = {
		0, 1, 2, 2, 3, 0,
		2, 1, 0, 0, 3, 2,
	};
	const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
	for (auto e : cubeIndices) {
		indices.push_back(e + offset);
	}

	const Mesh::Mesh mesh = Mesh::Mesh(id, iboEnd, indices.size());
	meshList.insert({ id, mesh });

	glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
	vboEnd += vertecies.size() * sizeof(Vertex);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
	iboEnd += indices.size() * sizeof(GLushort);
}

void Renderer::CreateFloorMesh(const char* id, const Vector3F& scale, int subdivideCount)
{
  std::vector<Vertex> vertecies;
  vertecies.reserve((subdivideCount + 1) * (subdivideCount + 1));
  float tx = 0.0f, ty = 0.0f;
  for (float y = 0; y < subdivideCount + 1; ++y) {
	for (float x = 0; x < subdivideCount + 1; ++x) {
	  vertecies.push_back(CreateVertex(
		Position3F(2 * x / subdivideCount - 1, -2 * y / subdivideCount + 1, 0) * scale,
		Vector3F(0.0f, 0.0f, 1.0f),
		Position2F(tx, ty)
	  ));
#if 0
	  tx += 1.0f;
	  if (tx > 1.0f) {
		tx = 0.0f;
	  }
	}
	ty += 1.0f;
	if (ty > 1.0f) {
	  ty = 0.0f;
	}
#else
	  tx = (x + 1.0f) / subdivideCount;
	}
	tx = 0.0f;
	ty = (y + 1.0f) / subdivideCount;
#endif
  }

  std::vector<GLushort> indices;
  indices.reserve((subdivideCount * subdivideCount) * 2 * 3);
  const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
  for (int y = 0; y < subdivideCount; ++y) {
	const int y0 = (y + 0) * (subdivideCount + 1) + offset;
	const int y1 = (y + 1) * (subdivideCount + 1) + offset;
	for (int x = 0; x < subdivideCount; ++x) {
	  indices.push_back(y0 + (x + 0));
	  indices.push_back(y1 + (x + 0));
	  indices.push_back(y1 + (x + 1));
	  indices.push_back(y1 + (x + 1));
	  indices.push_back(y0 + (x + 1));
	  indices.push_back(y0 + (x + 0));
	}
  }

  Mesh::Mesh mesh = Mesh::Mesh(id, iboEnd, indices.size());
  {
	const auto itr = textureList.find("floor");
	if (itr != textureList.end()) {
	  mesh.texDiffuse = itr->second;
	}
  }
  {
	const auto itr = textureList.find("floor_nml");
	if (itr != textureList.end()) {
	  mesh.texNormal = itr->second;
	}
  }
  meshList.insert({ id, mesh });

  glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
  vboEnd += vertecies.size() * sizeof(Vertex);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
  iboEnd += indices.size() * sizeof(GLushort);
}

void Renderer::CreateAsciiMesh(const char* id)
{
  std::vector<Vertex> vertecies;
  vertecies.reserve(8 * 16 * 4);
  static const float rectVertecies[] = {
	 0,  0,           0.0f, 64.0f / 512.0f,
	16,  0, 32.0f / 512.0f, 64.0f / 512.0f,
	16, 32, 32.0f / 512.0f,           0.0f,
	 0, 32,           0.0f,           0.0f,
  };
  for (int y = 0; y < 8; ++y) {
	for (int x = 0; x < 16; ++x) {
	  for (int i = 0; i < 4 * 4; i += 4) {
		vertecies.push_back(CreateVertex(
		  Position3F(rectVertecies[i + 0], rectVertecies[i + 1], 0.0f),
		  Vector3F(0.0f, 0.0f, 1.0f),
		  Position2F(rectVertecies[i + 2] + x * 32.0f / 512.0f, rectVertecies[i + 3] + (1.0f - (y + 1) * 64.0f / 512.0f))
		));
	  }
	}
  }

  std::vector<GLushort> indices;
  indices.reserve(8 * 16 * 6);
  static const GLushort rectIndices[] = {
	0, 1, 2, 2, 3, 0,
  };
  const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
  for (int y = 0; y < 8; ++y) {
	for (int x = 0; x < 16; ++x) {
	  for (auto e : rectIndices) {
		indices.push_back((y * 16 + x) * 4 + e + offset);
	  }
	}
  }
  const Mesh::Mesh mesh = Mesh::Mesh(id, iboEnd, indices.size());
  meshList.insert({ id, mesh });

  glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
  vboEnd += vertecies.size() * sizeof(Vertex);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
  iboEnd += indices.size() * sizeof(GLushort);
}

/** 雲のメッシュを作成し、メッシュリストに登録する.

  @param id メッシュのID. 他のメッシュと被らないユニークなIDを指定すること.
  @param scale  雲の大きさ(単位:メートル).
*/
void Renderer::CreateCloudMesh(const char* id, const Vector3F& scale)
{
  std::vector<Vertex> vertecies;
  std::vector<GLushort> indices;
  vertecies.reserve(4 * 10);
  indices.reserve(3 * 2 * 10);

  static const Vector3F boxelSize(40, 40, 40);
  const int wx = std::max(1, static_cast<int>(scale.x / boxelSize.x));
  const int wy = std::max(1, static_cast<int>(scale.y / boxelSize.y));
  const int wz = std::max(1, static_cast<int>(scale.z / boxelSize.z));
  std::vector<int> buf(wy * wz * wx, 0);
  {
	const int sequenceMax = std::max(1, (wy + wz + wx + 2) / 3);
	const int cloudCount = std::max(1, (wy * wz * wx * 6) / 10);
	const int startPos =
	  boost::random::uniform_int_distribution<>(wz / 4, wz / 2)(random) * wx +
	  boost::random::uniform_int_distribution<>(wx / 4, wx / 2)(random);
	buf[startPos] = 1;
	int pos = startPos;
	int sequence = 0;
	for (int i = 0; i < cloudCount; ++i) {
	  int d[6];
	  int ds = 0;
	  const int y = (pos / (wz * wx)) % wy;
	  const int z = (pos / wx) % wz;
	  const int x = pos % wx;
	  /* +y */ if (y < wy - 1 && !buf[pos + wz * wx]) { d[ds++] = 0; }
	  /* -y */ if (y > 0 && !buf[pos - wz * wx]) { d[ds++] = 1; }
	  /* +z */ if (z < wz - 1 && !buf[pos + wx]) { d[ds++] = 2; }
	  /* -z */ if (z > 0 && !buf[pos - wx]) { d[ds++] = 3; }
	  /* +x */ if (x < wx - 1 && !buf[pos + 1]) { d[ds++] = 4; }
	  /* -x */ if (x > 0 && !buf[pos - 1]) { d[ds++] = 5; }
	  if (ds == 0) {
		pos = startPos;
		sequence = 0;
	  } else {
		const int index = boost::random::uniform_int_distribution<>(0, ds - 1)(random);
		switch (d[index]) {
		case 0: pos += wz * wx; break;
		case 1: pos -= wz * wx; break;
		case 2: pos += wz; break;
		case 3: pos -= wz; break;
		case 4: ++pos; break;
		default: --pos; break;
		}
		buf[pos] = 1;
		++sequence;
		if (sequence >= sequenceMax) {
		  pos = startPos;
		  sequence = 0;
		}
	  }
	}
  }
  for (int i = 0; i < (wz * wx * 3) / 10; ++i) {
	const int pos =
	  boost::random::uniform_int_distribution<>(wz / 4, wz / 2)(random) * wx +
	  boost::random::uniform_int_distribution<>(wx / 4, wx / 2)(random);
	if (buf[pos]) {
	  buf[pos] = 3;
	}
  }

  {
	static const float basePos[][2] = { { -1, -1 },{ 1, -1 },{ 1, 1 },{ -1, 1 } };
	const Quaternion rot1(Vector3F(1, 0, 0), degreeToRadian<float>(-90));
	const Position3F baseOffset(-boxelSize.x * wx * 0.5f, boxelSize.y * 0.5f, -boxelSize.z * wz * 0.5f);
	Position3F offset;
	offset.z = baseOffset.z;
	for (int z = wz - 1; z >= 0; --z) {
	  offset.y = baseOffset.y;
	  for (int y = 0; y < wy; ++y) {
		offset.x = baseOffset.x;
		for (int x = 0; x < wx; ++x) {
		  const int hasCloud = buf[y * wz * wx + z * wx + x];
		  if (!hasCloud) {
			continue;
		  }
		  const float cloudType = static_cast<float>(hasCloud == 3 ? 3 : boost::random::uniform_int_distribution<>(0, 2)(random));
		  const Vector2F cloudScale = Vector2F(boxelSize.x, boxelSize.y) * (boost::random::uniform_int_distribution<>(85, 100)(random) * 0.01f);
		  const Quaternion rot0 = rot1 * Quaternion(Vector3F(0, 0, 1), degreeToRadian<float>(static_cast<float>(hasCloud == 3 ? 0 : boost::random::uniform_int_distribution<>(-45, 45)(random))));
		  const Vector3F offsetRnd(
			static_cast<float>(boost::random::uniform_int_distribution<>(static_cast<int>(boxelSize.x * 0.25f), static_cast<int>(boxelSize.x * 0.75f))(random)),
			static_cast<float>(boost::random::uniform_int_distribution<>(static_cast<int>(boxelSize.y * 0.25f), static_cast<int>(boxelSize.y * 0.75f))(random)),
			static_cast<float>(boost::random::uniform_int_distribution<>(static_cast<int>(boxelSize.z * 0.25f), static_cast<int>(boxelSize.z * 0.75f))(random))
		  );
		  for (int i = 0; i < 4; ++i) {
			Vector3F pos(basePos[i][0] * cloudScale.x, basePos[i][1] * cloudScale.y, 0);
			pos = rot0.Apply(pos);
			pos += offsetRnd;
			const Position2F texcoord((basePos[i][0] + 1) * 0.125f + cloudType * 0.25f, 0.75f + (basePos[i][1] + 1) * 0.125f);
			vertecies.push_back(CreateVertex(
			  offset + pos,
			  basePos[i][1] == 1 ? Vector3F(0.0f, 0.0f, 1.0f) : Vector3F(0.0f, 0.0f, 1.0f),
			  texcoord
			));
		  }
		  offset.x += boxelSize.x;
		}
		offset.y += boxelSize.y;
	  }
	  offset.z += boxelSize.z;
	}
#if 0
	{
	  float offsetX = 10.0f;
	  float offsetY = 0.0f;
	  for (int y = 0; y < middleCountY; ++y) {
		for (int x = 0; x < middleCountX; ++x) {
		  const float unitSize = boost::random::uniform_int_distribution<>(20, 29)(random);
		  const float cloudType = boost::random::uniform_int_distribution<>(0, 2)(random);
		  const Quaternion rot0(Vector3F(0, 0, 1), degreeToRadian<float>(boost::random::uniform_int_distribution<>(-450, 450)(random) / 10.0f));
		  for (int i = 0; i < 4; ++i) {
			Vector3F pos(basePos[i][0] * unitSize, basePos[i][1] * unitSize, 0);
			pos = rot1.Apply(rot0.Apply(pos));
			const Position2F texcoord((basePos[i][0] + 1) * 0.125f + cloudType * 0.25f, 0.75f + (basePos[i][1] + 1) * 0.125f);
			vertecies.push_back(CreateVertex(
			  Position3F(offsetX, offsetY + unitSize, 0) + pos,
			  Vector3F(0.0f, 0.0f, 1.0f),
			  texcoord
			  ));
		  }
		  offsetX += boost::random::uniform_int_distribution<>(20, 30)(random);
		}
		offsetY += boost::random::uniform_int_distribution<>(10, 20)(random);
	  }
	}
#endif
  }

  const GLushort offset = static_cast<GLushort>(vboEnd / sizeof(Vertex));
  for (GLushort i = 0; i < vertecies.size(); i += 4) {
	indices.push_back(i + 0 + offset);
	indices.push_back(i + 1 + offset);
	indices.push_back(i + 2 + offset);
	indices.push_back(i + 2 + offset);
	indices.push_back(i + 3 + offset);
	indices.push_back(i + 0 + offset);
  }

  Mesh::Mesh mesh = Mesh::Mesh(id, iboEnd, indices.size());
  {
	const auto itr = textureList.find("cloud");
	if (itr != textureList.end()) {
	  mesh.texDiffuse = itr->second;
	}
  }
  meshList.insert({ id, mesh });

  glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
  vboEnd += vertecies.size() * sizeof(Vertex);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
  iboEnd += indices.size() * sizeof(GLushort);
}

void Renderer::InitTexture()
{
	for (int i = FBO_Begin; i < FBO_End; ++i) {
	  const auto fboInfo = GetFBOInfo(i);
	  textureList.insert({ fboInfo.name, Texture::CreateEmpty2D(fboInfo.width, fboInfo.height) }); // HDR
	}

	textureList.insert({ "dummyCubeMap", Texture::CreateDummyCubeMap() });
	textureList.insert({ "dummy", Texture::CreateDummy2D() });
	textureList.insert({ "dummy_nml", Texture::CreateDummyNormal() });
	textureList.insert({ "ascii", Texture::LoadKTX("Textures/Common/ascii.ktx") });

	textureList.insert({ "wood", Texture::LoadKTX((texBaseDir + "wood.ktx").c_str()) });
	textureList.insert({ "wood_nml", Texture::LoadKTX((texBaseDir + "woodNR.ktx").c_str()) });
	textureList.insert({ "Sphere", Texture::LoadKTX((texBaseDir + "sphere.ktx").c_str()) });
	textureList.insert({ "Sphere_nml", Texture::LoadKTX((texBaseDir + "sphereNR.ktx").c_str()) });
//	textureList.insert({ "floor", Texture::LoadKTX((texBaseDir + "floor.ktx").c_str()) });
//	textureList.insert({ "floor_nml", Texture::LoadKTX((texBaseDir + "floorNR.ktx").c_str()) });
	textureList.insert({ "floor", Texture::LoadKTX("Textures/Common/landscape.ktx") });
	textureList.insert({ "floor_nml", Texture::LoadKTX("Textures/Common/landscapeNR.ktx") });

	textureList.insert({ "ls_coast", Texture::LoadKTX("Textures/Common/coast.ktx") });
	textureList.insert({ "ls_coast_nml", Texture::LoadKTX("Textures/Common/coastNR.ktx") });
	textureList.insert({ "building00", Texture::LoadKTX("Textures/Common/building00.ktx") });
	textureList.insert({ "building00_nml", Texture::LoadKTX((texBaseDir + "building00NR.ktx").c_str()) });
	textureList.insert({ "building01", Texture::LoadKTX("Textures/Common/building01.ktx") });
	textureList.insert({ "building01_nml", Texture::LoadKTX((texBaseDir + "building01NR.ktx").c_str()) });
	textureList.insert({ "tower00", Texture::LoadKTX("Textures/Common/tower00.ktx") });
	textureList.insert({ "tower00_nml", Texture::LoadKTX((texBaseDir + "tower00NR.ktx").c_str()) });

	textureList.insert({ "EggPack", Texture::LoadKTX("Textures/Common/EggPack.ktx") });
	textureList.insert({ "EggPack_nml", Texture::LoadKTX((texBaseDir + "EggPackNR.ktx").c_str()) });

	textureList.insert({ "flyingrock", Texture::LoadKTX((texBaseDir + "flyingrock.ktx").c_str()) });
	textureList.insert({ "flyingrock_nml", Texture::LoadKTX((texBaseDir + "flyingrockNR.ktx").c_str()) });
	textureList.insert({ "block1", Texture::LoadKTX("Textures/Common/block1.ktx") });
	textureList.insert({ "block1_nml", Texture::LoadKTX("Textures/Common/block1NR.ktx") });
	textureList.insert({ "chickenegg", Texture::LoadKTX((texBaseDir + "chickenegg.ktx").c_str()) });
	textureList.insert({ "chickenegg_nml", Texture::LoadKTX((texBaseDir + "chickeneggNR.ktx").c_str()) });
	textureList.insert({ "sunnysideup", Texture::LoadKTX("Textures/Common/SunnySideUp.ktx") });
	textureList.insert({ "sunnysideup_nml", Texture::LoadKTX((texBaseDir + "SunnySideUpNR.ktx").c_str()) });
	textureList.insert({ "sunnysideup01", Texture::LoadKTX("Textures/Common/SunnySideUp01.ktx") });
	textureList.insert({ "sunnysideup01_nml", Texture::LoadKTX((texBaseDir + "SunnySideUp01NR.ktx").c_str()) });
	textureList.insert({ "sunnysideup02", Texture::LoadKTX("Textures/Common/SunnySideUp02.ktx") });
	textureList.insert({ "sunnysideup02_nml", Texture::LoadKTX((texBaseDir + "SunnySideUp02NR.ktx").c_str()) });
	textureList.insert({ "sunnysideup03", Texture::LoadKTX("Textures/Common/SunnySideUp03.ktx") });
	textureList.insert({ "sunnysideup03_nml", Texture::LoadKTX((texBaseDir + "SunnySideUp03NR.ktx").c_str()) });
	textureList.insert({ "sunnysideup04", Texture::LoadKTX("Textures/Common/SunnySideUp04.ktx") });
	textureList.insert({ "sunnysideup04_nml", Texture::LoadKTX((texBaseDir + "SunnySideUp04NR.ktx").c_str()) });
	textureList.insert({ "overmedium", Texture::LoadKTX("Textures/Common/OverMedium.ktx") });
	textureList.insert({ "overmedium_nml", Texture::LoadKTX((texBaseDir + "OverMediumNR.ktx").c_str()) });
	textureList.insert({ "brokenegg", Texture::LoadKTX((texBaseDir + "brokenegg.ktx").c_str()) });
	textureList.insert({ "brokenegg_nml", Texture::LoadKTX((texBaseDir + "brokeneggNR.ktx").c_str()) });
	textureList.insert({ "flyingpan", Texture::LoadKTX((texBaseDir + "flyingpan.ktx").c_str()) });
	textureList.insert({ "flyingpan_nml", Texture::LoadKTX((texBaseDir + "flyingpanNR.ktx").c_str()) });
	textureList.insert({ "accelerator", Texture::LoadKTX((texBaseDir + "accelerator.ktx").c_str()) });
	textureList.insert({ "accelerator_nml", Texture::LoadKTX((texBaseDir + "acceleratorNR.ktx").c_str()) });
	textureList.insert({ "rock_s", Texture::LoadKTX((texBaseDir + "rock_s.ktx").c_str()) });
	textureList.insert({ "rock_s_nml", Texture::LoadKTX((texBaseDir + "rock_s_NR.ktx").c_str()) });
	textureList.insert({ "cloud", Texture::LoadKTX("Textures/Common/cloud.ktx") });
	textureList.insert({ "titlelogo", Texture::LoadKTX("Textures/Common/titlelogo.ktx") });
	textureList.insert({ "titlelogo_nml", Texture::LoadKTX("Textures/Common/titlelogoNR.ktx") });
	textureList.insert({ "font", Texture::LoadKTX("Textures/Common/font.ktx") });
	textureList.insert({ "checkpoint", Texture::LoadKTX("Textures/Common/CheckPoint.ktx") });
	textureList.insert({ "checkpoint_nml", Texture::LoadKTX((texBaseDir + "CheckPointNR.ktx").c_str()) });

	LoadLandscape(LandscapeOfScene_Default, TimeOfScene_Noon);
}

/**
* Load the textures for IBL.
*/
void Renderer::LoadLandscape(LandscapeOfScene type, TimeOfScene time) {
  static const char* const nameList[] = {
	"Landscape",
	"Coast",
  };
  static const char* const timeList[] = {
	"noon",
	"sunset",
	"night",
  };

  UnloadLandscape();
  const bool decompressing = texBaseDir != "Textures/Adreno/";
  const std::string iblSourcePath = std::string("Textures/IBL/") + nameList[type] + "/ibl_";
  for (char i = '1'; i <= '7'; ++i) {
	iblSpecularSourceList[i - '1'] = Texture::LoadKTX((iblSourcePath + timeList[time] + "_" + i + ".ktx").c_str(), decompressing);
  }
  iblDiffuseSourceList = Texture::LoadKTX((iblSourcePath + timeList[time] + "Irr.ktx").c_str(), decompressing);

  hasIBLTextures = true;
}

/**
* Unload the textures for IBL.
*/
void Renderer::UnloadLandscape() {
  hasIBLTextures = false;

  for (char i = '1'; i <= '7'; ++i) {
	iblSpecularSourceList[i - '1'].reset();
  }
  iblDiffuseSourceList.reset();
}

ObjectPtr Renderer::CreateObject(const char* meshName, const Material& m, const char* shaderName, ShadowCapability sc)
{
  Mesh::Mesh* pMesh = nullptr;
	auto mesh = meshList.find(meshName);
	if (mesh != meshList.end()) {
		pMesh = &mesh->second;
	} else {
	  LOGI("Mesh '%s' not found.", meshName);
	}
	Shader* pShader = nullptr;
	auto shader = shaderList.find(shaderName);
	if (shader != shaderList.end()) {
		pShader = &shader->second;
	} else {
	  LOGI("Shader '%s' not found.", shaderName);
	}
	return ObjectPtr(new Object(this, RotTrans::Unit(), pMesh, m, pShader, sc));
}

const Animation* Renderer::GetAnimation(const char* name)
{
	auto itr = animationList.find(name);
	if (itr != animationList.end()) {
	  return &itr->second;
	}
	return nullptr;
}

/** Set the color of filter.

  @param color  the color that is used by the fade-in or fade-out filter.
*/
void Renderer::SetFilterColor(const Color4B& color) {
  filterColor.r = color.r;
  filterColor.g = color.g;
  filterColor.b = color.b;
}

/** Get the color of filter.

  @return the color that is used by the fade-in or fade-out filter.

  @sa FadeIn().
*/
const Color4B& Renderer::GetFilterColor() const {
  return filterColor;
}

/** Start the fade-out filter.

  @param color the color that is used by the fade-in or fade-out filter.
  @param time  the fade-out time(the unit is seconds).

  @sa FadeIn().
*/
void Renderer::FadeOut(const Color4B& color, float time) {
  SetFilterColor(color);
  filterColor.a = 0;
  filterTimer = 0.0f;
  filterTargetTime = time;
  filterMode = FILTERMODE_FADEOUT;
}

/** Start the fade-in filter.

  the start color is setting by FadeOut() or SetFilterColor().

  @param time  the fade-out time(the unit is seconds).

  @sa FadeOut(), SetFilterColor().
*/
void Renderer::FadeIn(float time) {
  filterColor.a = 255;
  filterTimer = 0.0f;
  filterTargetTime = time;
  filterMode = FILTERMODE_FADEIN;
}

/** Get the curent filter mode.

  @return the current filter mode.

  @sa FadeIn(), FadeOut().
*/
Renderer::FilterMode Renderer::GetCurrentFilterMode() const {
  return filterMode;
}

/** Get the mesh object.

  @param id  An identifier string of the mesh object.

  @return A pointer to the mesh object if it exist in the renderer,
          otherwise nullptr.
*/
const Mesh::Mesh* Renderer::GetMesh(const std::string& id) const {
  auto itr = meshList.find(id);
  if (itr != meshList.end()) {
	return &itr->second;
  }
  return nullptr;
}

/** Get the shader object.

  @param id  An identifier string of the shader object.

  @return A pointer to the shader object if it exist in the renderer,
          otherwise nullptr.
*/
const Shader* Renderer::GetShader(const std::string& id) const {
  auto itr = shaderList.find(id);
  if (itr != shaderList.end()) {
	return &itr->second;
  }
  return nullptr;
}

} // namespace Mai;