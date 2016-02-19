#include "Renderer.h"
#include "Mesh.h"
#include "TangentSpaceData.h"
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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <numeric>
#include <streambuf>
#include <math.h>
#include <chrono>

namespace Mai {

namespace BPT = boost::property_tree;

#ifdef __ANDROID__
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "AndroidProject1.NativeActivity", __VA_ARGS__))
#else
#define LOGI(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#define LOGE(...) ((void)printf(__VA_ARGS__), (void)printf("\n"))
#endif // __ANDROID__

#define FBO_MAIN_WIDTH 512.0
#define FBO_MAIN_HEIGHT 800.0
#define MAIN_RENDERING_PATH_WIDTH 360.0
#define MAIN_RENDERING_PATH_HEIGHT 640.0
#define FBO_SUB_WIDTH 128.0
#define FBO_SUB_HEIGHT 128.0
#define SHADOWMAP_MAIN_WIDTH 512.0
#define SHADOWMAP_MAIN_HEIGHT 512.0
#define SHADOWMAP_SUB_WIDTH (SHADOWMAP_MAIN_WIDTH * 0.25)
#define SHADOWMAP_SUB_HEIGHT (SHADOWMAP_MAIN_HEIGHT * 0.25)

#define MAX_FONT_RENDERING_COUNT 256

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

	GLuint LoadShader(GLenum shaderType, const char* path) {
		GLuint shader = 0;
		if (auto buf = FileSystem::LoadFile(path)) {
			static const GLchar version[] = "#version 100\n";
#define MAKE_DEFINE_0(str, val) "#define " str " " #val "\n"
#define MAKE_DEFINE(def) MAKE_DEFINE_0(#def, def)
			static const GLchar defineList[] =
#ifdef SUNNYSIDEUP_DEBUG
			  "#define DEBUG\n"
#endif // SUNNYSIDEUP_DEBUG
			  MAKE_DEFINE(FBO_MAIN_WIDTH)
			  MAKE_DEFINE(FBO_MAIN_HEIGHT)
			  MAKE_DEFINE(FBO_SUB_WIDTH)
			  MAKE_DEFINE(FBO_SUB_HEIGHT)
			  MAKE_DEFINE(MAIN_RENDERING_PATH_WIDTH)
			  MAKE_DEFINE(MAIN_RENDERING_PATH_HEIGHT)
			  MAKE_DEFINE(SHADOWMAP_MAIN_WIDTH)
			  MAKE_DEFINE(SHADOWMAP_MAIN_HEIGHT)
			  MAKE_DEFINE(SHADOWMAP_SUB_WIDTH)
			  MAKE_DEFINE(SHADOWMAP_SUB_HEIGHT)
#ifdef USE_HDR_BLOOM
			  "#define USE_HDR_BLOOM\n"
#endif // USE_HDR_BLOOM
			  "#define SCALE_BONE_WEIGHT(w) ((w) * (1.0 / 255.0))\n"
			  "#define SCALE_TEXCOORD(c) ((c) * (1.0 / 65535.0))\n"
			  ;
#undef MAKE_DEFINE
#undef MAKE_DEFINE_0
			shader = glCreateShader(shaderType);
			if (!shader) {
				return 0;
			}
			const GLchar* pSrc[] = {
			  version,
			  defineList,
			  reinterpret_cast<GLchar*>(static_cast<void*>(&(*buf)[0])),
			};
			const GLint srcSize[] = {
			  sizeof(version) - 1,
			  sizeof(defineList) - 1,
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

	boost::optional<Shader> CreateShaderProgram(const char* name, const char* vshPath, const char* fshPath) {
		GLuint vertexShader = LoadShader(GL_VERTEX_SHADER, vshPath);
		if (!vertexShader) {
			return boost::none;
		}

		GLuint pixelShader = LoadShader(GL_FRAGMENT_SHADER, fshPath);
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
		s.texDiffuse = glGetUniformLocation(program, "texDiffuse");
		s.texNormal = glGetUniformLocation(program, "texNormal");
		s.texMetalRoughness = glGetUniformLocation(program, "texMetalRoughness");
		s.texIBL = glGetUniformLocation(program, "texIBL");
		s.texShadow = glGetUniformLocation(program, "texShadow");
		s.texSource = glGetUniformLocation(program, "texSource");
		s.unitTexCoord = glGetUniformLocation(program, "unitTexCoord");
		s.matView = glGetUniformLocation(program, "matView");
		s.matProjection = glGetUniformLocation(program, "matProjection");
		s.matLightForShadow = glGetUniformLocation(program, "matLightForShadow");
		s.bones = glGetUniformLocation(program, "boneMatrices");

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
  if (!mesh->jointList.empty()) {
	std::vector<Mai::RotTrans> rtList = animationPlayer.Update(mesh->jointList, t);
	UpdateJointRotTrans(rtList, &mesh->jointList[0], &mesh->jointList[0], Mai::RotTrans::Unit());
	std::transform(rtList.begin(), rtList.end(), bones.begin(), [this](const Mai::RotTrans& rt) { return ToMatrix(this->rotTrans * rt); });
  }
}

/** コンストラクタ.
*/
Renderer::Renderer()
  : isInitialized(false)
  , texBaseDir("Textures/Others/")
  , random(static_cast<uint32_t>(time(nullptr)))
  , fboMain(0)
  , fboSub(0)
  , fboShadow0(0)
  , fboShadow1(0)
  , depth(0)
  , filterMode(FILTERMODE_NONE)
  , filterColor(0, 0, 0, 0)
  , filterTimer(0.0f)
{
  for (auto& e : fboHDR) {
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
	static const char* const  fboNameList[] = {
		"fboMain",
		"fboSub",
		"fboShadow0",
		"fboShadow1",
		"fboHDR0",
		"fboHDR1",
		"fboHDR2",
		"fboHDR3",
		"fboHDR4",
	};
	GLuint* p = const_cast<GLuint*>(&fboMain + id);
	return { fboNameList[id], p };
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
	{
	  LOGI("GL_EXTENTIONS:");
	  const char* p = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	  const std::vector<std::string> v = split<std::string>(p, ' ', [](const std::string& s) { return s; });
	  for (const auto& e : v) {
		LOGI("  %s", e.c_str());
		if (e == "GL_NV_fence") {
		  hasNVfenceExtension = true;
		}
	  }
	}
#undef MAKE_TEX_ID_PAIR
#undef LOG_SHADER_INFO

	InitNVFenceExtention(hasNVfenceExtension);

	glGetIntegerv(GL_VIEWPORT, viewport);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);

	static const char* const shaderNameList[] = {
	  "default",
	  "defaultWithAlpha",
	  "default2D",
	  "cloud",
	  "skybox",
	  "shadow",
	  "bilinear4x4",
	  "gaussian3x3",
	  "sample4",
	  "reduceLum",
	  "hdrdiff",
	  "applyhdr",
	  "tbn",
	  "font",
	};
	for (const auto e : shaderNameList) {
		const std::string vert = std::string("Shaders/") + std::string(e) + std::string(".vert");
		const std::string frag = std::string("Shaders/") + std::string(e) + std::string(".frag");
		if (boost::optional<Shader> s = CreateShaderProgram(e, vert.c_str(), frag.c_str())) {
			shaderList.insert({ s->id, *s });
		}
	}

	InitTexture();

	{
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 1024 * 10, 0, GL_STATIC_DRAW);
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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 1024 * 10 * 3, 0, GL_STATIC_DRAW);
		iboEnd = 0;
#ifdef SHOW_TANGENT_SPACE
		glGenBuffers(1, &vboTBN);
		glBindBuffer(GL_ARRAY_BUFFER, vboTBN);
		glBufferData(GL_ARRAY_BUFFER, sizeof(TBNVertex) * 1024 * 30, 0, GL_STATIC_DRAW);
		vboTBNEnd = 0;
#endif // SHOW_TANGENT_SPACE
		InitMesh();

		LOGI("VBO: %lu%%", vboEnd * 100 / (sizeof(Vertex) * 1024 * 10));
		LOGI("IBO: %lu%%", iboEnd * 100 / (sizeof(GLushort) * 1024 * 10 * 3));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	{
		const auto& tex = *textureList["fboMain"];
		glGenFramebuffers(1, &fboMain);
		glGenRenderbuffers(1, &depth);
		glBindRenderbuffer(GL_RENDERBUFFER, depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, tex.Width(), tex.Height());
		glBindFramebuffer(GL_FRAMEBUFFER, fboMain);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex.TextureId());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.TextureId(), 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOGE("Error: FrameBufferObject(fboMain) is not complete!\n");
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteRenderbuffers(1, &depth);
			glDeleteFramebuffers(1, &fboMain);
			glBindTexture(GL_TEXTURE_2D, 0);
			fboMain = depth = 0;
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
  const Mesh& mesh = meshList["ascii"];
  float x = pos.x;
  for (const char* p = str; *p; ++p) {
	Matrix4x4 mMV = Matrix4x4::FromScale(0.5f, 0.5f, 1.0f);
	mMV = mV * Matrix4x4::Translation(x, pos.y, 0) * mMV;
	glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mMV.f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset + *p * 6 * sizeof(GLushort)));
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
*/
void Renderer::AddString(float x, float y, float scale, const Color4B& color, const char* str) {
  const size_t freeCount = MAX_FONT_RENDERING_COUNT - vboFontEnd / sizeof(FontVertex);
  if (strlen(str) > freeCount) {
	return;
  }
  std::vector<FontVertex> vertecies;
  Position2F curPos = Position2F(x, y) * Vector2F(2, -2) + Vector2F(-1, 1);
  while (const char c = *(str++)) {
	const FontInfo& info = GetAsciiFontInfo(c);
	const float w = info.GetWidth() * (2.0f / viewport[2]) * scale;
	const float h = info.GetHeight() * (-2.0f / viewport[3]) * scale;
	vertecies.push_back({ curPos, info.leftTop, color });
	vertecies.push_back({ Position2F(curPos.x, curPos.y + h), Position2S(info.leftTop.x, info.rightBottom.y), color });
	vertecies.push_back({ Position2F(curPos.x + w, curPos.y), Position2S(info.rightBottom.x, info.leftTop.y), color });
	vertecies.push_back({ Position2F(curPos.x + w, curPos.y + h), info.rightBottom, color });
	curPos.x += w;
  }
  glBindBuffer(GL_ARRAY_BUFFER, vboFont[currentFontBufferNo]);
  glBufferSubData(GL_ARRAY_BUFFER, vboFontEnd, vertecies.size() * sizeof(FontVertex), &vertecies[0]);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  fontRenderingInfoList.push_back({ static_cast<GLint>(vboFontEnd / sizeof(FontVertex)), static_cast<GLsizei>(vertecies.size()) });
  vboFontEnd += vertecies.size() * sizeof(FontVertex);
}

/** Get string width on screen.
*/
float Renderer::GetStringWidth(const char* str) const {
  float w = 0.0f;
  const float d = 1.0f / static_cast<float>(viewport[2]);
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
  const float d = 1.0f / static_cast<float>(viewport[3]);
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

  glUniform1i(shader.texDiffuse, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureList["font"]->TextureId());

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
  for (auto e : fontRenderingInfoList) {
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
//	if (begin == end) {
//		return;
//	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	// とりあえず適当なカメラデータからビュー行列を設定.
	const Position3F eye = cameraPos;
	const Position3F at = cameraPos + cameraDir;
	const Matrix4x4 mView = LookAt(eye, at, cameraUp);

	static float fov = 60.0f;

	// パフォーマンス計測準備.
	static const int FENCE_ID_SHADOW_PATH = 0;
	static const int FENCE_ID_SHADOW_FILTER_PATH = 1;
	static const int FENCE_ID_COLOR_PATH = 2;
	static const int FENCE_ID_HDR_PATH = 3;
	static const int FENCE_ID_FINAL_PATH = 4;
	GLuint fences[5];
	Local::glGenFencesNV(5, fences);

	// shadow path.

	static const float shadowNear = 10.0f;
	static const float shadowFar = 2000.0f; // 精度を確保するため短めにしておく.
	const Matrix4x4 mViewL = LookAt(eye + Vector3F(200, 500, 200), eye, Vector3F(0, 1, 0));
//	const Matrix4x4 mProjL = Perspective(fov, SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT, shadowNear, shadowFar);
	const Matrix4x4 mProjL = Olthographic(SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT, shadowNear, shadowFar);
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
	  static float ms = 0.5f;// 4.0f;// transformedCenter.w / frustumRadius;
	  static float mss = 2.25f;
	  const float mx = -transformedCenter.x / transformedCenter.w * mss;
	  const float my = -transformedCenter.y / transformedCenter.w * mss;
	  const Matrix4x4 m = { {
	    ms,  0,  0,  0,
	     0, ms,  0,  0,
	     0,  0,  1,  0,
		mx, my,  0,  1,
	  } };
	  mCropL = m;
	}
	const Matrix4x4 mVPForShadow = mCropL * mProjL * mViewL;

#if 1
	{
		glEnableVertexAttribArray(VertexAttribLocation_Position);
		glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
		glDisableVertexAttribArray(VertexAttribLocation_Normal);
		glDisableVertexAttribArray(VertexAttribLocation_Tangent);
		glDisableVertexAttribArray(VertexAttribLocation_TexCoord01);
		glEnableVertexAttribArray(VertexAttribLocation_Weight);
		glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
		glEnableVertexAttribArray(VertexAttribLocation_BoneID);
		glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);

		glBindFramebuffer(GL_FRAMEBUFFER, fboMain);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_ONE, GL_ZERO);
		glCullFace(GL_FRONT);

		glViewport(0, 0, static_cast<GLsizei>(SHADOWMAP_MAIN_WIDTH), static_cast<GLsizei>(SHADOWMAP_MAIN_HEIGHT));
		glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const Shader& shader = shaderList["shadow"];
		glUseProgram(shader.program);
		glUniformMatrix4fv(shader.matLightForShadow, 1, GL_FALSE, mVPForShadow.f);

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
			  const Matrix4x3 m = mScale * ToMatrix(obj.RotTrans());
			  glUniform4fv(shader.bones, 3, m.f);
			}
			glDrawElements(GL_TRIANGLES, obj.Mesh()->iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(obj.Mesh()->iboOffset));
		}
		// 自分の影(とりあえず球体で代用).
		if (1) {
		  Matrix4x3 m = Matrix4x3::Unit();
		  m.Set(0, 3, eye.x);
		  m.Set(1, 3, eye.y);
		  m.Set(2, 3, eye.z);
		  glUniform4fv(shader.bones, 3, m.f); // 平行移動を含まないMatrix4x4はMatrix4x3の代用になりうるけどお行儀悪い.
		  const Mesh& mesh = meshList["Sphere"];
		  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
		}
		{
		  Matrix4x4 m = Matrix4x4::RotationX(degreeToRadian(90.0f));
		  // 地面は裏面を持たないため、-Y方向にオフセットすることで裏面とする.
		  {
			glCullFace(GL_BACK);
			m.Set(1, 3, -0.01f);
		  }
		  glUniform4fv(shader.bones, 3, m.f); // 平行移動を含まないMatrix4x4はMatrix4x3の代用になりうるけどお行儀悪い.
		  const Mesh& mesh = meshList["ground"];
		  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
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
		  const Mesh& mesh = meshList["Sphere"];
		  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
		}

		Local::glSetFenceNV(fences[FENCE_ID_SHADOW_PATH], GL_ALL_COMPLETED_NV);
	}

	// fboMain ->(bilinear4x4)-> fboShadow0
	{
		glEnableVertexAttribArray(VertexAttribLocation_Position);
		glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
		glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
		glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);

		glBindFramebuffer(GL_FRAMEBUFFER, fboShadow0);
		glViewport(0, 0, static_cast<GLsizei>(SHADOWMAP_SUB_WIDTH), static_cast<GLsizei>(SHADOWMAP_SUB_HEIGHT));
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE, GL_ZERO);
		glCullFace(GL_BACK);

		const Shader& shader = shaderList["bilinear4x4"];
		glUseProgram(shader.program);

		Matrix4x4 mtx = Matrix4x4::Unit();
		mtx.Scale(1.0f, -1.0f, 1.0f);
		glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

		glUniform1i(shader.texShadow, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureList["fboMain"]->TextureId());

		const Mesh& mesh = meshList["board2D"];
		glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	}

	// fboShadow0 ->(gaussian3x3)-> fboShadow1
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fboShadow1);
		glViewport(0, 0, static_cast<GLsizei>(SHADOWMAP_SUB_WIDTH), static_cast<GLsizei>(SHADOWMAP_SUB_HEIGHT));
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_ONE, GL_ZERO);
		glCullFace(GL_BACK);

		const Shader& shader = shaderList["gaussian3x3"];
		glUseProgram(shader.program);

		Matrix4x4 mtx = Matrix4x4::Unit();
		mtx.Scale(1.0f, -1.0f, 1.0f);
		glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

		glUniform1i(shader.texShadow, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureList["fboShadow0"]->TextureId());

		const Mesh& mesh = meshList["board2D"];
		glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));

		Local::glSetFenceNV(fences[FENCE_ID_SHADOW_FILTER_PATH], GL_ALL_COMPLETED_NV);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
#endif

	// color path.

	glBindFramebuffer(GL_FRAMEBUFFER, fboMain);
	glViewport(0, 0, static_cast<GLsizei>(MAIN_RENDERING_PATH_WIDTH), static_cast<GLsizei>(MAIN_RENDERING_PATH_HEIGHT));
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
	  MAIN_RENDERING_PATH_WIDTH,
	  MAIN_RENDERING_PATH_HEIGHT,
	  1.0f,
	  5000.0f
	);

	// 適当にライトを置いてみる.
	const Vector4F lightPos(50, 50, 50, 1.0);
	const Vector3F lightColor(7000.0f, 6000.0f, 5000.0f);
#if 1
	{
	  const Shader& shader = shaderList["skybox"];
	  glUseProgram(shader.program);

	  Matrix4x4 mTrans(Matrix4x4::Unit());
	  mTrans.SetVector(3, Vector4F(eye.x, eye.y, eye.z, 1));
	  const Matrix4x4 m = mView * mTrans;
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
	  glUniformMatrix4fv(shader.matView, 1, GL_FALSE, m.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_high"]->TextureId());
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
	  const Mesh& mesh = meshList["skybox"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	  Local::glSetFenceNV(fences[FENCE_ID_COLOR_PATH], GL_ALL_COMPLETED_NV);
	}

	GLuint currentProgramId = 0;
	for (const ObjectPtr* itr = begin; itr != end; ++itr) {
		const Object& obj = *itr->get();
		if (!obj.IsValid()) {
			continue;
		}

		const Shader& shader = *obj.Shader();
		if (shader.program && shader.program != currentProgramId) {
			glUseProgram(shader.program);
			currentProgramId = shader.program;

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

			// IBL用テクスチャを設定.
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_high"]->TextureId());
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_low"]->TextureId());
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["irradiance"]->TextureId());
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, textureList["fboShadow1"]->TextureId());

			if (shader.program == shaderList["cloud"].program) {
				glDepthMask(GL_FALSE);
				glDisable(GL_CULL_FACE);
			} else {
				glDepthMask(GL_TRUE);
				glEnable(GL_CULL_FACE);
			}
#ifdef SUNNYSIDEUP_DEBUG
			static float debug = -1;
			glUniform1f(shader.debug, debug);
#endif // SUNNYSIDEUP_DEBUG
		}

		const Vector4F materialColor = obj.Color().ToVector4F();
		glUniform4fv(shader.materialColor, 1, &materialColor.x);
		const float metallic = obj.Metallic();
		const float roughness = obj.Roughness();
		glUniform2f(shader.materialMetallicAndRoughness, metallic, roughness);

		{
			const auto& texDiffuse = obj.Mesh()->texDiffuse;
			glActiveTexture(GL_TEXTURE0);
			if (texDiffuse) {
				glBindTexture(GL_TEXTURE_2D, texDiffuse->TextureId());
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			const auto& texNormal = obj.Mesh()->texNormal;
			glActiveTexture(GL_TEXTURE1);
			if (texNormal) {
				glBindTexture(GL_TEXTURE_2D, texNormal->TextureId());
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
		  const Matrix4x3 m = mScale * ToMatrix(obj.RotTrans());
		  glUniform4fv(shader.bones, 3, m.f);
		}
		glDrawElements(GL_TRIANGLES, obj.Mesh()->iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(obj.Mesh()->iboOffset));
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
		  const Matrix4x3 m = mScale * ToMatrix(obj.RotTrans());
		  glUniform4fv(shader.bones, 3, m.f);
		}
		glDrawArrays(GL_LINES, obj.Mesh()->vboTBNOffset, obj.Mesh()->vboTBNCount);
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
	  glBindFramebuffer(GL_FRAMEBUFFER, fboSub);
	  glViewport(0, 0, static_cast<GLsizei>(MAIN_RENDERING_PATH_WIDTH) / 4, static_cast<GLsizei>(MAIN_RENDERING_PATH_HEIGHT) / 4);
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
	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	}
	// fboSub ->(hdrdiff)-> fboHDR[0]
	{
	  glBindFramebuffer(GL_FRAMEBUFFER, fboHDR[0]);
	  glViewport(0, 0, static_cast<GLsizei>(MAIN_RENDERING_PATH_WIDTH) / 4, static_cast<GLsizei>(MAIN_RENDERING_PATH_HEIGHT) / 4);

	  const Shader& shader = shaderList["hdrdiff"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboSub"]->TextureId());
	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	}
	// fboHDR[0] ->(sample4)-> fboHDR[1] ... fboHDR[4]
	{
	  const Shader& shader = shaderList["sample4"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);

	  const Mesh& mesh = meshList["board2D"];
	  int scale = 8;
	  for (int i = FBO_HDR0; i < FBO_HDR4; ++i, scale *= 2) {
		glBindFramebuffer(GL_FRAMEBUFFER, *GetFBOInfo(i + 1).p);
		glViewport(0, 0, static_cast<GLsizei>(MAIN_RENDERING_PATH_WIDTH) / scale, static_cast<GLsizei>(MAIN_RENDERING_PATH_HEIGHT) / scale);
		glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 0.25f / static_cast<float>(MAIN_RENDERING_PATH_WIDTH / scale), 0.25f / static_cast<float>(MAIN_RENDERING_PATH_HEIGHT / scale));
		glBindTexture(GL_TEXTURE_2D, textureList[GetFBOInfo(i).name]->TextureId());
		glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	  }
	}
	// fboHDR[4] ->(default2D)-> fboHDR[3] ... fboHDR[0]
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
	  const Mesh& mesh = meshList["board2D"];
	  int scale = 32;
	  for (int i = FBO_HDR4; i > FBO_HDR0; --i, scale /= 2) {
		glBindFramebuffer(GL_FRAMEBUFFER, *GetFBOInfo(i - 1).p);
		glViewport(0, 0, static_cast<GLsizei>(MAIN_RENDERING_PATH_WIDTH) / scale, static_cast<GLsizei>(MAIN_RENDERING_PATH_HEIGHT) / scale);
		glBindTexture(GL_TEXTURE_2D, textureList[GetFBOInfo(i).name]->TextureId());
		glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
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
	  glActiveTexture(GL_TEXTURE2);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboHDR0"]->TextureId());
	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));

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
	  Matrix4x4 mV = LookAt(Vector3F(0, 0, 10), Vector3F(0, 0, 0), Vector3F(0, 1, 0));
	  Matrix4x4 mMV = Matrix4x4::Unit();
	  mMV.Scale(128.0f, 128.0f, 0.0f);
	  mMV = mV * Matrix4x4::Translation(128, 128 + 8 + 56, 0) * mMV;
	  glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mMV.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboShadow1"]->TextureId());
	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
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
	BPT::ptree::const_iterator findElement(const BPT::ptree& pt, const std::string& element, const std::string& attr, const std::string& id) {
		return std::find_if(
			pt.begin(),
			pt.end(),
			[&element, &attr, &id](const BPT::ptree::value_type& t) -> bool { return t.first == element && t.second.get<std::string>(attr) == id; }
		);
	}

	std::string getSourceArray(const BPT::ptree& pt, const std::string& id, const char* subElemName) {
		const BPT::ptree::const_iterator itr = findElement(pt, "source", "<xmlattr>.id", id);
		if (itr == pt.end()) {
			return std::string();
		}
		return itr->second.get<std::string>(subElemName);
	}
	std::string getSourceArray(const BPT::ptree& pt, const std::string& id) {
		return getSourceArray(pt, id, "float_array");
	}
	std::string getSourceNameArray(const BPT::ptree& pt, const std::string& id) {
		return getSourceArray(pt, id, "Name_array");
	}

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

void Renderer::LoadFBX(const char* filename, const char* diffuse, const char* normal)
{
  if (auto pBuf = FileSystem::LoadFile(filename)) {
	ImportMeshResult result = ImportMesh(*pBuf, vbo, vboEnd, ibo, iboEnd);
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
  }
}

/** 描画に必要なポリゴンメッシュの初期化.
* ポリゴンメッシュを読み込み、バッファオブジェクトに格納する.
* Initialize()から呼び出されるため、明示的に呼び出す必要はない.
*/
void Renderer::InitMesh()
{
	LoadMesh("cubes.dae", TangentSpaceData::defaultData, "wood", "wood_nml");
	LoadMesh("sphere.dae", TangentSpaceData::sphereData, "Sphere", "Sphere_nml");
	LoadMesh("flyingrock.dae", TangentSpaceData::flyingrockData, "flyingrock", "flyingrock_nml");
//	LoadMesh("block1.dae", TangentSpaceData::block1Data, "block1", "block1_nml");
//	LoadMesh("chickenegg.dae", TangentSpaceData::chickeneggData, "chickenegg", "chickenegg_nml");
	LoadMesh("sunnysideup.dae", TangentSpaceData::sunnysideupData, "sunnysideup", "sunnysideup_nml");
//	LoadMesh("flyingpan.dae", TangentSpaceData::flyingpanData, "flyingpan", "flyingpan_nml");
	LoadMesh("brokenegg.dae", TangentSpaceData::brokeneggData, "brokenegg", "brokenegg_nml");
	LoadMesh("accelerator.dae", TangentSpaceData::acceleratorData, "accelerator", "accelerator_nml");

	LoadFBX("Meshes/block1.msh", "block1", "block1_nml");
	LoadFBX("Meshes/flyingpan.msh", "flyingpan", "flyingpan_nml");
	LoadFBX("Meshes/chickenegg.msh", "chickenegg", "chickenegg_nml");
	LoadFBX("Meshes/titlelogo.msh", "titlelogo", "titlelogo_nml");

	CreateSkyboxMesh();
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

	Mesh mesh = Mesh("skybox", iboEnd, indices.size());
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

  Mesh mesh = Mesh("octahedron", iboEnd, indices.size());
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

	const Mesh mesh = Mesh(id, iboEnd, indices.size());
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

  Mesh mesh = Mesh(id, iboEnd, indices.size());
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
  const Mesh mesh = Mesh(id, iboEnd, indices.size());
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

  Mesh mesh = Mesh(id, iboEnd, indices.size());
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

void Renderer::LoadMesh(const char* filename, const void* pTD, const char* texDiffuse, const char* texNormal)
{
	if (auto pBuf = FileSystem::LoadFile(filename)) {
		// property_treeを使ってCOLLADAファイルを解析.
		FileSystem::RawBufferType& buf = *pBuf;
		struct membuf : public std::streambuf {
			explicit membuf(char* b, char* e) { this->setg(b, b, e); }
		};
		membuf mb((char*)&buf[0], (char*)&buf[0] + buf.size());
		std::istream is(&mb);
		BPT::ptree collada;
		try {
			BPT::read_xml(is, collada);
			LOGI("XML READ SUCCESS %s", filename);
		}
		catch (BPT::xml_parser_error& e) {
			LOGI("%s", e.message().c_str());
		}
		std::vector<std::string> boneNameList;
		try {
			/* library_geometries.geometory.mesh.triangles
			<xmlattr>.count           三角形の数.
			input.<xmlattr>.semantic  入力の種類(VERTEX/NORMAL/TEXCOORD)
			input.<xmlattr>.source    入力元データID. mesh.sourceのうちIDの一致するもの.
			input.<xmlattr>.offset    各ソースに対応するインデックス配列内のオフセット.
			input.<xmlattr>.set       同じセマンティックの入力が複数ある場合に、それぞれを識別するためのインデックス.
			とりあえずTEXCOORDのみ対応する.
			*/
			/* すべての入力のインデックスを連結したものを、その頂点のIDとする.
			新しいインデックスによるIDについて、既存の頂点IDに一致するものがあれば、既存の頂点を採用し
			新しい頂点を生成しない.
			*/
			/* library_controllers.controller
			<xmlattr>.id  node要素のinstance_controller.<xmlattr>url属性で指定されるID.
			skin
			  <xmlattr>.source                この頂点ウェイトに対応するメッシュID.
			  joints.input.<xmlattr>.semantic 入力の種類(JOINT/INV_BIND_MATRIX).
			                                  JOINTの入力データは対応するジョイントを示す文字列で、アニメーションデータと
											  関連付けるために使われる.
			  joints.input.<xmlattr>.source   入力元データID. skin.sourceのうちIDの一致するもの.
			  vertex_weight.<xmlattr>.count          頂点ウェイトデータの数.
			  vertex_weight.input.<xmlattr>.semantic 入力の種類(JOINT/WEIGHT).
			  vertex_weight.input.<xmlattr>.source   入力元データID. skin.sourceのうちIDの一致するもの.
			  vertex_weight.input.<xmlattr>.offset   各ソースに対応するインデックス配列内のオフセット.
			  vertex_weight.vcount                   各頂点に結び付けられるデータ数.
			  vertex_weight.v                        inputに対応するインデックス配列.

			頂点ウェイトデータは暗黙的に頂点座標に結び付けられるようだ.
			頂点ウェイトデータとメッシュデータの関連付けは、NODE型のnode要素の子要素であるinstance_controller.<xmlattr>.urlと
			instance_material.<xmlattr>.symbolによって行われるが、頂点ウェイトデータの数とメッシュの頂点座標の数は一致していな
			ければならないようだ(ウェイトと座標を明示的に結びつけるデータを持たないぽい).

			libyrary_visual_scene.visual_scene
			  node
			    <xmlattr>.type
				  NODEの場合、ジョイントとメッシュを関連付けるデータとして扱う. 下記のデータを参照する.
			      instance_controller
				    <xmlattr>.url  頂点ウェイトデータのURL.
				    skeleton インスタンスに関連付けられるジョイントノードを指定する.
			                 この要素はヒントのようなもので、簡単なモデルの場合は無くても動作するようだ.

				  JOINTの場合、ジョイントノードのツリーとして扱う.
				  <xmlattr>.sid            アニメーションデータの適用先を識別するために使われる.
				  translate, rotate, scale ジョイントの基本姿勢データ.
				                           また、各要素の<xmlattr>sidは、アニメーションデータの適用先を識別するために使われる.

			COLLADAのアニメーションは、独立したアニメーションデータをチャンネルで指定された適用先に流し込む、というイメージ.
			channelがsamplerを指定しsamplerは複数のデータ列からなるアニメーション入力ソースを指定する.
			各アニメーションデータは1次元のパラメータしか持たないことに注意. つまり、3軸回転のためには3つのチャンネルが必要となる.
			COLLADAの仕様では3軸に個別にキーフレームを振ることも可能だが、普通は3軸ともに共通の値が使われると思うので、実装でも3軸
			をまとめて扱うことにする(というか、回転をクォータニオンに変換して球面線形補間したい).
			あと適用先IDは当然ながら文字列になってて、ジョイント側のIDが一致する要素が適用先になる. なので、厳密にアニメーションを
			解釈しようとするなら、ジョイント側の座標変換に関わる要素を全て保持しておいて、一致する要素の軸で回転させたり、移動させ
			たりする必要がある. のだが、面倒なので適用先IDは決め打ちにする.
			TODO: 気が向いたらちゃんと実装しよう.

			animation.channel
			  <xmlattr>.source アニメーションサンプラID.
			  <xmlattr>.target アニメーションの適用先ID.
			animation.sampler
			  <xmlattr>.id  channel.<xmlattr>.sourceに対応するID.
			  input 入力ソース.
			*/
			bool fromBlender = false;
			if (const auto& upAxisNode = collada.get_child_optional("COLLADA.asset.up_axis")) {
				if (upAxisNode->data() == "Z_UP") {
					fromBlender = true;
				}
			}
			const BPT::ptree& scenesNode = collada.get_child("COLLADA.library_visual_scenes.visual_scene");
			struct SkinInfo {
				std::vector<Matrix4x4> invBindPoseMatrixList;
				std::vector<std::array<int, 4> >  boneIdList;
				std::vector<std::array<float, 4> >  weightList;
				std::string meshId;
			};
			static const size_t defaultMaxVertexCount = 10000;
			std::vector<Vertex> vertecies;
			std::vector<GLushort> indices;
			vertecies.reserve(defaultMaxVertexCount);
			indices.reserve(defaultMaxVertexCount * 3);
			GLintptr baseIndexOffset = iboEnd;
#ifdef SHOW_TANGENT_SPACE
			GLintptr baseVertexOffsetTBN = vboTBNEnd;
#endif // SHOW_TANGENT_SPACE
			int baseVertexOffset = vboEnd / sizeof(Vertex);
			for (auto& e : scenesNode) {
				if (e.first != "node" || e.second.get<std::string>("<xmlattr>.type") != "NODE") {
					continue;
				}
				const BPT::ptree& nodeElem = e.second;
				Vector3F nodeBaseScale(1, 1, 1);
				if (auto opt = nodeElem.get_optional<std::string>("scale")) {
					const std::vector<float> list = split<float>(opt.get(), ' ', [](const std::string& s) { return atof(s.c_str()); });
					if (list.size() >= 3) {
						nodeBaseScale.x = list[0];
						nodeBaseScale.y = list[1];
						nodeBaseScale.z = list[2];
					}
				}
				Vector3F nodeBaseTranslate(0, 0, 0);
				if (auto opt = nodeElem.get_optional<std::string>("translate")) {
				  const std::vector<float> list = split<float>(opt.get(), ' ', [](const std::string& s) { return atof(s.c_str()); });
				  if (list.size() >= 3) {
					nodeBaseTranslate.x = list[0];
					nodeBaseTranslate.y = list[1];
					nodeBaseTranslate.z = list[2];
				  }
				}
				SkinInfo skinInfo;
				const BPT::ptree::const_assoc_iterator itrControllerUrl = e.second.find("instance_controller");
				if (itrControllerUrl != e.second.not_found()) {
					const std::string contollerId = itrControllerUrl->second.get<std::string>("<xmlattr>.url").substr(1);
					const BPT::ptree& controllersElem = collada.get_child("COLLADA.library_controllers");
					for (auto& e : controllersElem) {
						if (e.first != "controller" || e.second.get<std::string>("<xmlattr>.id") != contollerId) {
							continue;
						}
						const BPT::ptree& skinElem = e.second.get_child("skin");
						skinInfo.meshId = skinElem.get<std::string>("<xmlattr>.source").substr(1);
						const BPT::ptree& jointsElem = skinElem.get_child("joints");
						for (auto& e : jointsElem) {
							if (e.first != "input") {
								continue;
							}
							const std::string semantic = e.second.get<std::string>("<xmlattr>.semantic");
							const std::string srcId = e.second.get<std::string>("<xmlattr>.source").substr(1);
							if (semantic == "JOINT") {
								// TODO: skin配下のsource要素から一致するデータを検索し、アニメーション連携用のジョイント名配列として保存する.
								boneNameList = split<std::string>(getSourceNameArray(skinElem, srcId), ' ', [](const std::string& s) { return s; });
							} else if (semantic == "INV_BIND_MATRIX") {
								// TODO: skin配下のsource要素から一致するデータを検索し、ボーンオフセット行列として保存する.
								//       ボーン本体はJOINT型node要素なのに、このデータがskin要素に置かれているのは奇妙に思えるが、この行列が必要
								//       なのはスキニングを行う場合に(ほぼ)限られることを考えれば、それほど不思議ではない.
								const std::vector<float> tmp = split<float>(getSourceArray(skinElem, srcId), ' ', [](const std::string& s) { return atof(s.c_str()); });
								for (size_t i = 0; tmp.size() - i >= 16; i += 16) {
									Matrix4x4 m;
									std::copy(tmp.begin() + i, tmp.begin() + i + 16, m.f);
									skinInfo.invBindPoseMatrixList.push_back(m);
								}
							}
						}
						const BPT::ptree& weightsElem = skinElem.get_child("vertex_weights");
						int boneIdOffset = 0, weightOffset = 0;
						std::vector<float> weightList;
						for (auto& e : weightsElem) {
							if (e.first != "input") {
								continue;
							}
							const std::string semantic = e.second.get<std::string>("<xmlattr>.semantic");
							const std::string sourceId = e.second.get<std::string>("<xmlattr>.source").substr(1);
							if (semantic == "JOINT") {
								boneIdOffset = e.second.get<int>("<xmlattr>.offset");
							} else if (semantic == "WEIGHT") {
								weightOffset = e.second.get<int>("<xmlattr>.offset");
								weightList = split<float>(getSourceArray(skinElem, sourceId), ' ', [](const std::string& s) { return atof(s.c_str()); });
							}
						}
						const std::vector<int> vcount = split<int>(weightsElem.get<std::string>("vcount"), ' ', [](const std::string& s) { return atoi(s.c_str()); });
						const std::vector<int> indexList = split<int>(weightsElem.get<std::string>("v"), ' ', [](const std::string& s) { return atoi(s.c_str()); });
						size_t offset = 0;
						for (auto& e : vcount) {
							std::array<int, 4> boneId = { { 0, 0, 0, 0 } };
							std::array<float, 4> weight = { { 0, 0, 0, 0 } };
							for (int i = 0; i < e; ++i) {
								boneId[i] = indexList[offset + 2 * i + boneIdOffset];
								weight[i] = weightList[indexList[offset + 2 * i + weightOffset]];
							}
							skinInfo.boneIdList.push_back(boneId);
							skinInfo.weightList.push_back(weight);
							offset += 4 * 2;
							if (offset >= indexList.size()) {
								break;
							}
						}
					}
				} else {
					const BPT::ptree::const_assoc_iterator itrGeometryUrl = e.second.find("instance_geometry");
					if (itrGeometryUrl == e.second.not_found()) {
						continue;
					}
					skinInfo.meshId = itrGeometryUrl->second.get<std::string>("<xmlattr>.url").substr(1);
				}
				const BPT::ptree& libGeoElem = collada.get_child("COLLADA.library_geometries");
				const BPT::ptree::const_iterator itr = std::find_if(libGeoElem.begin(), libGeoElem.end(), [&skinInfo](const BPT::ptree::value_type& v) { return v.first == "geometry" && v.second.get<std::string>("<xmlattr>.id") == skinInfo.meshId; });
				if (itr == libGeoElem.end()) {
					LOGI("Geometory '%s' not found.", skinInfo.meshId.c_str());
					continue;
				}
				const BPT::ptree& meshNode = itr->second.get_child("mesh");
				const BPT::ptree* pTriangleNode;
				if (auto p = meshNode.get_child_optional("triangles")) {
					pTriangleNode = &p.get();
				}  else if (auto p = meshNode.get_child_optional("polylist")) {
					pTriangleNode = &p.get();
				} else {
					LOGI("%s has not triangles or polylist.", nodeElem.get<std::string>("<xmlattr>.id").c_str());
					continue;
				}
				const BPT::ptree& trianglesNode = *pTriangleNode;

				// input要素を解析し、対応するソース及びオフセットを得る.
				std::vector<Position3F> posArray;
				std::vector<Vector3F> normalArray;
				typedef std::pair<int, Position2S> TexCoordType;
				std::vector<TexCoordType> texcoordArray[VERTEX_TEXTURE_COUNT_MAX];
				int vertexOff = -1, normalOff = -1, texcoordOff[VERTEX_TEXTURE_COUNT_MAX] = { -1, -1 };
				int offsetMax = 0;
				for (auto& e : trianglesNode) {
					if (e.first != "input") {
						continue;
					}
					const std::string semantic = e.second.get<std::string>("<xmlattr>.semantic");
					if (semantic == "VERTEX") {
						LOGI("VERTEX FOUND");
						const std::string vertSrcId = e.second.get<std::string>("<xmlattr>.source").substr(1);
						LOGI("VERTEX SOURCE:%s", vertSrcId.c_str());
						std::string srcId;
						for (auto& e : meshNode) {
							if (e.first == "vertices" &&
								e.second.get<std::string>("<xmlattr>.id") == vertSrcId &&
								e.second.get<std::string>("input.<xmlattr>.semantic") == "POSITION") {
								srcId = e.second.get<std::string>("input.<xmlattr>.source").substr(1);
								LOGI("POSITION SOURCE: #%s", vertSrcId.c_str());
								break;
							}
						}
						const std::vector<float> tmp = split<float>(getSourceArray(meshNode, srcId), ' ', [](const std::string& s) -> float { return static_cast<float>(atof(s.c_str())); });
						posArray.shrink_to_fit();
						posArray.reserve(tmp.size() / 3);
						for (size_t i = 0; i + 2 < tmp.size(); i += 3) {
							posArray.push_back(Position3F(tmp[i], tmp[i + 1], tmp[i + 2]) * nodeBaseScale + nodeBaseTranslate);
						}
						LOGI("POS ARRAY SIZE:%d", posArray.size());
						vertexOff = e.second.get<int>("<xmlattr>.offset");
						offsetMax = std::max(offsetMax, vertexOff);
					}
					else if (semantic == "NORMAL") {
						LOGI("NORMAL FOUND");
						const std::string srcId = e.second.get<std::string>("<xmlattr>.source").substr(1);
						LOGI("NORMAL SOURCE: #%s", srcId.c_str());
						const std::vector<float> tmp = split<float>(getSourceArray(meshNode, srcId), ' ', [](const std::string& s) -> float { return static_cast<float>(atof(s.c_str())); });
						normalArray.shrink_to_fit();
						normalArray.reserve(tmp.size() / 3);
						for (size_t i = 0; i + 2 < tmp.size(); i += 3) {
							normalArray.push_back(Vector3F(tmp[i], tmp[i + 1], tmp[i + 2]));
						}
						LOGI("NORMAL ARRAY SIZE:%d", normalArray.size());
						normalOff = e.second.get<int>("<xmlattr>.offset");
						offsetMax = std::max(offsetMax, normalOff);
					}
					else if (semantic == "TEXCOORD") {
						LOGI("TEXCOORD FOUND");
						const unsigned int index = e.second.get<unsigned int>("<xmlattr>.set", 0);
						if (index < VERTEX_TEXTURE_COUNT_MAX) {
							const std::string srcId = e.second.get<std::string>("<xmlattr>.source").substr(1);
							LOGI("TEXCOORD[%d] SOURCE: #%s", index, srcId.c_str());
							const std::vector<float> tmp = split<float>(getSourceArray(meshNode, srcId), ' ', [](const std::string& s) -> float { return static_cast<float>(atof(s.c_str())); });
							auto& list = texcoordArray[index];
							list.shrink_to_fit();
							list.reserve(tmp.size() / 2);
							for (size_t i = 0; i + 1 < tmp.size(); i += 2) {
								// 既に同じ座標データが存在していたら、そのデータのインデックスを自分のインデックスとして記録する.
							    const Position2S uv = Position2S::FromFloat(tmp[i], tmp[i + 1]);
								const auto& itr = std::find_if(list.begin(), list.end(), [uv](const TexCoordType& obj) { return obj.second == uv; });
								const int id = (itr == list.end()) ? (i / 2) : itr->first;
								list.push_back(std::make_pair(id, uv));
							}
							LOGI("TEXCOORD[%d] ARRAY SIZE:%d", index, list.size());
							texcoordOff[index] = e.second.get<int>("<xmlattr>.offset");
							offsetMax = std::max(offsetMax, texcoordOff[index]);
						}
					}
				}
				const int stride = offsetMax + 1; // 頂点ごとに必要とするインデックスの数.
				LOGI("STRIDE:%d", stride);

				const int indexCount = trianglesNode.get<int>("<xmlattr>.count") * 3; // trianglesノードの場合、count要素は三角形の個数を示すので、インデックスの要素数にするため3を掛けている.
				std::vector<uint_fast64_t> vertIdList;
				vertIdList.reserve(indexCount);
				const std::vector<int> indexList = split<int>(trianglesNode.get<std::string>("p"), ' ', [](const std::string& s)->int { return atoi(s.c_str()); });
				for (size_t i = 0; i < indexList.size(); i += stride) {
					const uint_fast64_t posId64 = indexList[i + vertexOff];
					const uint_fast64_t idNormal = (normalOff != -1) ? indexList[i + normalOff] : 0xfff;
					uint_fast64_t idTexcoord[VERTEX_TEXTURE_COUNT_MAX];
					for (int j = 0; j < VERTEX_TEXTURE_COUNT_MAX; ++j) {
					  if (texcoordOff[j] != -1) {
						const int tmpIndex = indexList[i + texcoordOff[j]];
						idTexcoord[j] = texcoordArray[j][tmpIndex].first;
					  } else {
						idTexcoord[j] = 0xfff;
					  }
					}
					const uint_fast64_t vertId = (posId64 << 0) | (idNormal << 12) | (idTexcoord[0] << 24) | (idTexcoord[1] << 36);
					const size_t posId = static_cast<size_t>(posId64);
					// LOGI("vertId: %llx", vertId);
					auto itr = std::find(vertIdList.begin(), vertIdList.end(), vertId);
					if (itr != vertIdList.end()) {
						indices.push_back(itr - vertIdList.begin() + baseVertexOffset);
					} else {
						Vertex v;
						v.position = (vertexOff != -1) ? posArray[posId] : Position3F(0, 0, 0);
						v.normal = (normalOff != -1) ? normalArray[static_cast<size_t>(idNormal)] : Vector3F(0, 0, 1);
						v.tangent = Vector3F(0, 0, 0);
						for (int j = 0; j < VERTEX_TEXTURE_COUNT_MAX; ++j) {
							v.texCoord[j] = (texcoordOff[j] != -1) ? texcoordArray[j][static_cast<size_t>(idTexcoord[j])].second : Position2S(0, 0);
						}
						// TODO: transformにおける量子化の結果、合計が255(=1.0)にならないことがあるため、補正処理が必要.
						if (boneNameList.empty()) {
							static const GLubyte defaultWeight[] = { 255, 0, 0, 0 };
							static const GLubyte defaultBoneID[] = { 0, 0, 0, 0 };
							std::copy(defaultWeight, defaultWeight + 4, v.weight);
							std::copy(defaultBoneID, defaultBoneID + 4, v.boneID);
						} else {
							std::transform(skinInfo.weightList[posId].begin(), skinInfo.weightList[posId].end(), v.weight, [](float w) { return static_cast<GLubyte>(w * 255.0f + 0.5f); });
							std::copy(skinInfo.boneIdList[posId].begin(), skinInfo.boneIdList[posId].end(), v.boneID);
						}
						indices.push_back(static_cast<GLushort>(vertIdList.size() + baseVertexOffset));
						vertecies.push_back(v);
						vertIdList.push_back(vertId);
					}
				}
				// for (auto e : vertecies) {
				//	LOGI("VERT:%f,%f,%f", e.position.x, e.position.y, e.position.z);
				// }
				// for (int i = 0; indices.size() - i >= 3; i += 3) {
				//	LOGI("INDEX:%d, %d, %d", indices[i + 0], indices[i + 1], indices[i + 2]);
				// }

				// ジョイントノードツリーを取得する.
				struct local {
					static Joint* AddJoint(std::vector<Joint>& list, const BPT::ptree& pt, const std::vector<std::string>& nameList) {
						Joint* pJoint = nullptr;
						Joint* pFirstSibling = nullptr;
						for (const BPT::ptree::value_type& e : boost::make_iterator_range(pt.equal_range("node"))) {
							if (e.second.get<std::string>("<xmlattr>.type") != "JOINT") {
								continue;
							}
							const std::string sid = e.second.get<std::string>("<xmlattr>.sid");
							auto itr = std::find(nameList.begin(), nameList.end(), sid);
							if (itr == nameList.end()) {
								LOGI("JOINT '%s' is undefined in name list.", sid.c_str());
								break;
							}
							const int index = itr - nameList.begin();

							const std::vector<float> transList = split<float>(e.second.get<std::string>("translate"), ' ', [](const std::string& s) { return atof(s.c_str()); });
							RotTrans rotTrans = { Quaternion(0, 0, 0, 1), Vector3F(transList[0], transList[1], transList[2]) };
							for (auto&& r : boost::make_iterator_range(e.second.equal_range("rotate"))) {
								const std::vector<float> rotData = split<float>(r.second.data(), ' ', [](const std::string& s) { return atof(s.c_str()); });
								const Quaternion q(Vector3F(rotData[0], rotData[1], rotData[2]), degreeToRadian(rotData[3]));
								rotTrans.rot = q * rotTrans.rot;
							}
							Joint* pChild = AddJoint(list, e.second, nameList);
							Joint& j = list[index];
							j.initialPose = rotTrans;
							j.offChild = pChild ? pChild - &j : 0;
							j.offSibling = 0;
							if (pJoint) {
								pJoint->offSibling = &j - pJoint;
							} else {
								pFirstSibling = &j;
							}
							pJoint = &j;
						}
						return pFirstSibling;
					}
				};

				Mesh mesh = Mesh(nodeElem.get<std::string>("<xmlattr>.id"), baseIndexOffset, indexCount);
#ifdef SHOW_TANGENT_SPACE
				mesh.vboTBNOffset = baseVertexOffsetTBN / sizeof(TBNVertex);
				mesh.vboTBNCount = vertIdList.size() * 3 * 2;
				baseVertexOffsetTBN += vertIdList.size() * 3 * 2;
#endif // SHOW_TANGENT_SPACE
				if (!boneNameList.empty()) {
					std::vector<Joint> joints;
					joints.resize(boneNameList.size());
					for (size_t i = 0; i < joints.size(); ++i) {
						skinInfo.invBindPoseMatrixList[i].Decompose(&joints[i].invBindPose.rot, nullptr, &joints[i].invBindPose.trans);
					}
					local::AddJoint(joints, scenesNode, boneNameList);
					mesh.SetJoint(boneNameList, joints);
				}
				if (texDiffuse) {
					const auto itr = textureList.find(texDiffuse);
					if (itr != textureList.end()) {
						mesh.texDiffuse = itr->second;
					}
				}
				if (texNormal) {
					const auto itr = textureList.find(texNormal);
					if (itr != textureList.end()) {
						mesh.texNormal = itr->second;
					}
				}
				meshList.insert({ mesh.id, mesh });
				baseIndexOffset += sizeof(GLushort) * indexCount;
				baseVertexOffset += vertIdList.size();
			}

			// 軸変換
			if (fromBlender) {
				for (auto& e : vertecies) {
					e.position = Position3F(-e.position.x, e.position.z, e.position.y);
					e.normal = Vector3F(-e.normal.x, e.normal.z, e.normal.y).Normalize();
				}
			}

			// 頂点タンジェントを計算する.
			TangentSpaceData::CalculateTangent(static_cast<const TangentSpaceData::Data*>(pTD), indices, vboEnd, vertecies);

#ifdef SHOW_TANGENT_SPACE
			{
			  std::vector<TBNVertex> tbn;
			  tbn.reserve(vertecies.size() * 3 * 2);
			  static const Color4B red(255, 0, 0, 255), green(0, 255, 0, 255), blue(0, 0, 255, 255);
			  static const float lentgh = 0.2f;
			  for (auto& e : vertecies) {
				const Vector3F t = e.tangent.ToVec3();
				const Vector3F b = e.normal.Cross(t).Normalize() * e.tangent.w;
				tbn.push_back(TBNVertex(e.position, green, e.boneID, e.weight));
				tbn.push_back(TBNVertex(e.position + b * lentgh, green, e.boneID, e.weight));
				tbn.push_back(TBNVertex(e.position, red, e.boneID, e.weight));
				tbn.push_back(TBNVertex(e.position + t * lentgh, red, e.boneID, e.weight));
				tbn.push_back(TBNVertex(e.position, blue, e.boneID, e.weight));
				tbn.push_back(TBNVertex(e.position + e.normal * lentgh, blue, e.boneID, e.weight));
			  }
			  glBindBuffer(GL_ARRAY_BUFFER, vboTBN);
			  glBufferSubData(GL_ARRAY_BUFFER, vboTBNEnd, tbn.size() * sizeof(TBNVertex), &tbn[0]);
			  vboTBNEnd += tbn.size() * sizeof(TBNVertex);
			}
#endif // SHOW_TANGENT_SPACE

			LOGI("MAKING VERTEX LIST SUCCESS:%d, %d", vertecies.size(), indices.size());
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
			vboEnd += vertecies.size() * sizeof(Vertex);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
			iboEnd += indices.size() * sizeof(GLushort);
		}
		catch (BPT::ptree_bad_path& e) {
			LOGI("BAD PATH: %s", e.what());
		}

		// アニメーションデータを取得する.
		const boost::optional<BPT::ptree&> libAnimeElem = collada.get_child_optional("COLLADA.library_animations");
		if (libAnimeElem) {
			try {
				for (auto&& rootAnime : boost::make_iterator_range(libAnimeElem->equal_range("animation"))) {
					Animation animation;
					animation.id = rootAnime.second.get<std::string>("<xmlattr>.id");
					for (auto&& subAnime : boost::make_iterator_range(rootAnime.second.equal_range("animation"))) {
						const BPT::ptree& channelElem = subAnime.second.get_child("channel");
						const std::string srcId = channelElem.get<std::string>("<xmlattr>.source").substr(1);
						const BPT::ptree::const_iterator itrSampler = findElement(subAnime.second, "sampler", "<xmlattr>.id", srcId);
						if (itrSampler == subAnime.second.end()) {
							LOGI("Element not found: <sampler> in <channel source=\"#%s\">", srcId.c_str());
							continue;
						}
						const BPT::ptree::const_iterator itrInput = findElement(itrSampler->second, "input", "<xmlattr>.semantic", "INPUT");
						if (itrInput == itrSampler->second.end()) {
							LOGI("Element not found: <input semantic=\"INPUT\"> in <channel source=\"#%s\">", srcId.c_str());
							continue;
						}
						const BPT::ptree::const_iterator itrOutput = findElement(itrSampler->second, "input", "<xmlattr>.semantic", "OUTPUT");
						if (itrOutput == itrSampler->second.end()) {
							LOGI("Element not found: <input semantic=\"OUTPUT\"> in <channel source=\"%s\">", srcId.c_str());
							continue;
						}
						const std::vector<float> timeList = split<float>(getSourceArray(subAnime.second, itrInput->second.get<std::string>("<xmlattr>.source").substr(1)), ' ', [](const std::string& s) { return atof(s.c_str()); });
						const std::vector<float> angleList = split<float>(getSourceArray(subAnime.second, itrOutput->second.get<std::string>("<xmlattr>.source").substr(1)), ' ', [](const std::string& s) { return atof(s.c_str()); });
						const std::string targetPath = channelElem.get<std::string>("<xmlattr>.target");
						const size_t idSeparationPoint = targetPath.find_last_of('/');
						const size_t sidSeparationPoint = targetPath.find_last_of('.');
						const std::string jointId = targetPath.substr(0, idSeparationPoint);
						const std::string jointSid = targetPath.substr(idSeparationPoint + 1, sidSeparationPoint - (idSeparationPoint + 1));
						const std::string jointOption = targetPath.substr(sidSeparationPoint + 1);
						const int index = std::find(boneNameList.begin(), boneNameList.end(), jointId) - boneNameList.begin();
						auto itrData = std::find_if(animation.data.begin(), animation.data.end(), [index](const Animation::ElementList& n) { return n.first == index; });
						if (itrData == animation.data.end()) {
							animation.data.push_back(Animation::ElementList());
							itrData = animation.data.end() - 1;
							itrData->first = index;
						}
						const Vector3F axis = jointSid == "rotateX" ? Vector3F(1, 0, 0) : (jointSid == "rotateY" ? Vector3F(0, 1, 0) : Vector3F(0, 0, 1));
						for (size_t i = 0; i < timeList.size(); ++i) {
							const auto itr = std::lower_bound(itrData->second.begin(), itrData->second.end(), timeList[i], [](const Animation::Element& lhs, float rhs) { return lhs.time < rhs; });
							if (itr != itrData->second.end() && itr->time == timeList[i]) {
								itr->pose.rot *= Quaternion(axis, degreeToRadian(angleList[i]));
								itr->pose.rot.Normalize();
							} else {
								Animation::Element newElem = { { Quaternion(axis, degreeToRadian(angleList[i])), Vector3F(0, 0, 0) }, timeList[i] };
								newElem.pose.rot.Normalize();
								itrData->second.insert(itr, newElem);
							}
						}
					}
					animationList.insert({ animation.id, animation });
				}
			}
			catch (BPT::ptree_bad_path& e) {
				LOGI("BAD PATH: %s", e.what());
			}
		}
		LOGI("Success loading %s", filename);
	}
}

void Renderer::InitTexture()
{
	textureList.insert({ "fboMain", Texture::CreateEmpty2D(static_cast<int>(FBO_MAIN_WIDTH), static_cast<int>(FBO_MAIN_HEIGHT)) }); // 影初期描画 & カラー描画
	textureList.insert({ "fboSub", Texture::CreateEmpty2D(static_cast<int>(MAIN_RENDERING_PATH_WIDTH / 4), static_cast<int>(MAIN_RENDERING_PATH_HEIGHT / 4)) }); // カラー(1/4)
	textureList.insert({ "fboShadow0", Texture::CreateEmpty2D(static_cast<int>(SHADOWMAP_SUB_WIDTH), static_cast<int>(SHADOWMAP_SUB_HEIGHT)) }); // 影縮小
	textureList.insert({ "fboShadow1", Texture::CreateEmpty2D(static_cast<int>(SHADOWMAP_SUB_WIDTH), static_cast<int>(SHADOWMAP_SUB_HEIGHT)) }); // 影ぼかし
	int scale = 4;
	for (int i = FBO_HDR0; i <= FBO_HDR4; ++i) {
	  textureList.insert({ GetFBOInfo(i).name, Texture::CreateEmpty2D(static_cast<int>(MAIN_RENDERING_PATH_WIDTH / scale), static_cast<int>(MAIN_RENDERING_PATH_HEIGHT / scale)) }); // HDR
	  scale *= 2;
	}

	textureList.insert({ "dummyCubeMap", Texture::CreateDummyCubeMap() });
	textureList.insert({ "dummy", Texture::CreateDummy2D() });
	textureList.insert({ "ascii", Texture::LoadKTX("Textures/Common/ascii.ktx") });

	textureList.insert({ "skybox_high", Texture::LoadKTX((texBaseDir + "skybox_high.ktx").c_str()) });
	textureList.insert({ "skybox_low", Texture::LoadKTX((texBaseDir + "skybox_low.ktx").c_str()) });
	textureList.insert({ "irradiance", Texture::LoadKTX((texBaseDir + "irradiance.ktx").c_str()) });
	textureList.insert({ "wood", Texture::LoadKTX((texBaseDir + "wood.ktx").c_str()) });
	textureList.insert({ "wood_nml", Texture::LoadKTX((texBaseDir + "woodNR.ktx").c_str()) });
	textureList.insert({ "Sphere", Texture::LoadKTX((texBaseDir + "sphere.ktx").c_str()) });
	textureList.insert({ "Sphere_nml", Texture::LoadKTX((texBaseDir + "sphereNR.ktx").c_str()) });
//	textureList.insert({ "floor", Texture::LoadKTX((texBaseDir + "floor.ktx").c_str()) });
//	textureList.insert({ "floor_nml", Texture::LoadKTX((texBaseDir + "floorNR.ktx").c_str()) });
	textureList.insert({ "floor", Texture::LoadKTX("Textures/Common/landscape.ktx") });
	textureList.insert({ "floor_nml", Texture::LoadKTX((texBaseDir + "landscapeNR.ktx").c_str()) });
	textureList.insert({ "flyingrock", Texture::LoadKTX((texBaseDir + "flyingrock.ktx").c_str()) });
	textureList.insert({ "flyingrock_nml", Texture::LoadKTX((texBaseDir + "flyingrockNR.ktx").c_str()) });
	textureList.insert({ "block1", Texture::LoadKTX((texBaseDir + "block1.ktx").c_str()) });
	textureList.insert({ "block1_nml", Texture::LoadKTX((texBaseDir + "block1NR.ktx").c_str()) });
	textureList.insert({ "chickenegg", Texture::LoadKTX((texBaseDir + "chickenegg.ktx").c_str()) });
	textureList.insert({ "chickenegg_nml", Texture::LoadKTX((texBaseDir + "chickeneggNR.ktx").c_str()) });
	textureList.insert({ "sunnysideup", Texture::LoadKTX((texBaseDir + "sunnysideup.ktx").c_str()) });
	textureList.insert({ "sunnysideup_nml", Texture::LoadKTX((texBaseDir + "sunnysideupNR.ktx").c_str()) });
	textureList.insert({ "brokenegg", Texture::LoadKTX((texBaseDir + "brokenegg.ktx").c_str()) });
	textureList.insert({ "brokenegg_nml", Texture::LoadKTX((texBaseDir + "brokeneggNR.ktx").c_str()) });
	textureList.insert({ "flyingpan", Texture::LoadKTX((texBaseDir + "flyingpan.ktx").c_str()) });
	textureList.insert({ "flyingpan_nml", Texture::LoadKTX((texBaseDir + "flyingpanNR.ktx").c_str()) });
	textureList.insert({ "accelerator", Texture::LoadKTX((texBaseDir + "accelerator.ktx").c_str()) });
	textureList.insert({ "accelerator_nml", Texture::LoadKTX((texBaseDir + "acceleratorNR.ktx").c_str()) });
	textureList.insert({ "cloud", Texture::LoadKTX("Textures/Common/cloud.ktx") });
	textureList.insert({ "titlelogo", Texture::LoadKTX("Textures/Common/titlelogo.ktx") });
	textureList.insert({ "titlelogo_nml", Texture::LoadKTX("Textures/Common/titlelogoNR.ktx") });
	textureList.insert({ "font", Texture::LoadKTX("Textures/Common/font.ktx") });
}

ObjectPtr Renderer::CreateObject(const char* meshName, const Material& m, const char* shaderName)
{
	Mesh* pMesh = nullptr;
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
	return ObjectPtr(new Object(RotTrans::Unit(), pMesh, m, pShader));
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

} // namespace Mai;