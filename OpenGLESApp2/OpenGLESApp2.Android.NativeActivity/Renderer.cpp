#include "Renderer.h"
#include "android_native_app_glue.h"
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <streambuf>
#include <math.h>
#include <GLES2/gl2ext.h>

namespace BPT = boost::property_tree;

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "AndroidProject1.NativeActivity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "AndroidProject1.NativeActivity", __VA_ARGS__))

namespace {
	template<typename T>
	T degreeToRadian(T d) {
		return d * boost::math::constants::pi<T>() / static_cast<T>(180);
	}

	void checkGlError(const char* op) {
		for (GLint error = glGetError(); error; error = glGetError()) {
			LOGI("after %s() glError (0x%x)\n", op, error);
		}
	}

	typedef std::vector<uint8_t> RawBufferType;
	boost::optional<RawBufferType> LoadFile(android_app* state, const char* filename, int mode = 0)
	{
		RawBufferType buf;
		AAsset* pAsset = AAssetManager_open(state->activity->assetManager, filename, mode);
		if (!pAsset) {
			return boost::none;
		}
		const off_t size = AAsset_getLength(pAsset);
		buf.resize(size);
		const int result = AAsset_read(pAsset, &buf[0], AAsset_getLength(pAsset));
		AAsset_close(pAsset);
		if (result < 0) {
			return boost::none;
		}
		return buf;
	}

	GLuint LoadShader(android_app* state, GLenum shaderType, const char* path) {
		GLuint shader = 0;
		if (auto buf = LoadFile(state, path)) {
			shader = glCreateShader(shaderType);
			if (!shader) {
				return 0;
			}
			const GLchar* pSrc = reinterpret_cast<GLchar*>(static_cast<void*>(&(*buf)[0]));
			const GLint srcSize = buf->size();
			glShaderSource(shader, 1, &pSrc, &srcSize);
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
				if (buf.size() >= infoLen) {
					glGetShaderInfoLog(shader, infoLen, NULL, &buf[0]);
					LOGE("Could not compile shader %d:\n%s\n", shaderType, &buf[0]);
				}
				glDeleteShader(shader);
				return 0;
			}
		}
		return shader;
	}

	boost::optional<Shader> CreateShaderProgram(android_app* state, const char* name, const char* vshPath, const char* fshPath) {
		GLuint vertexShader = LoadShader(state, GL_VERTEX_SHADER, vshPath);
		if (!vertexShader) {
			return boost::none;
		}

		GLuint pixelShader = LoadShader(state, GL_FRAGMENT_SHADER, fshPath);
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
			glBindAttribLocation(program, VertexAttribLocation_TexCoord01, "vTexCoord01");
			glBindAttribLocation(program, VertexAttribLocation_Weight, "vWeight");
			glBindAttribLocation(program, VertexAttribLocation_BoneID, "vBoneID");
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

	Matrix4x4 LookAt(const Vector3F& eyePos, const Vector3F& targetPos, const Vector3F& upVector)
	{
		const Vector3F ezVector = (eyePos - targetPos).Normalize();
		const Vector3F exVector = upVector.Cross(ezVector);
		const Vector3F eyVector = ezVector.Cross(exVector);
		Matrix4x4 m;
		m.SetVector(0, Vector4F(exVector.x, eyVector.x, ezVector.x, 0));
		m.SetVector(1, Vector4F(exVector.y, eyVector.y, ezVector.y, 0));
		m.SetVector(2, Vector4F(exVector.z, eyVector.z, ezVector.z, 0));
		m.SetVector(3, Vector4F(-exVector.Dot(eyePos), -eyVector.Dot(eyePos), -ezVector.Dot(eyePos), 1.0f));
		return m;
	}
} // unnamed namespace

/** Shaderデストラクタ.
*/
Shader::~Shader()
{
}

Matrix4x4& Matrix4x4::Inverse()
{
  Matrix4x4 ret;
  float det_1;
  float pos = 0;
  float neg = 0;
  float temp;

  temp = f[0] * f[5] * f[10];
  if (temp >= 0)
	pos += temp;
  else
	neg += temp;
  temp = f[4] * f[9] * f[2];
  if (temp >= 0)
	pos += temp;
  else
	neg += temp;
  temp = f[8] * f[1] * f[6];
  if (temp >= 0)
	pos += temp;
  else
	neg += temp;
  temp = -f[8] * f[5] * f[2];
  if (temp >= 0)
	pos += temp;
  else
	neg += temp;
  temp = -f[4] * f[1] * f[10];
  if (temp >= 0)
	pos += temp;
  else
	neg += temp;
  temp = -f[0] * f[9] * f[6];
  if (temp >= 0)
	pos += temp;
  else
	neg += temp;
  det_1 = pos + neg;

  if (det_1 == 0.0) {
	//Error
  } else {
	det_1 = 1.0f / det_1;
	ret.f[0] =  (f[5] * f[10] - f[9] * f[6]) * det_1;
	ret.f[1] = -(f[1] * f[10] - f[9] * f[2]) * det_1;
	ret.f[2] =  (f[1] * f[ 6] - f[5] * f[2]) * det_1;
	ret.f[4] = -(f[4] * f[10] - f[8] * f[6]) * det_1;
	ret.f[5] =  (f[0] * f[10] - f[8] * f[2]) * det_1;
	ret.f[6] = -(f[0] * f[ 6] - f[4] * f[2]) * det_1;
	ret.f[8] =  (f[4] * f[ 9] - f[8] * f[5]) * det_1;
	ret.f[9] = -(f[0] * f[ 9] - f[8] * f[1]) * det_1;
	ret.f[10] = (f[0] * f[ 5] - f[4] * f[1]) * det_1;

	/* Calculate -C * inverse(A) */
	ret.f[12] = -(f[12] * ret.f[0] + f[13] * ret.f[4] + f[14] * ret.f[8]);
	ret.f[13] = -(f[12] * ret.f[1] + f[13] * ret.f[5] + f[14] * ret.f[9]);
	ret.f[14] = -(f[12] * ret.f[2] + f[13] * ret.f[6] + f[14] * ret.f[10]);

	ret.f[3] = 0.0f;
	ret.f[7] = 0.0f;
	ret.f[11] = 0.0f;
	ret.f[15] = 1.0f;
  }

  *this = ret;
  return *this;
}

/** クォータニオンを4x3行列に変換する.
*/
Matrix4x3 ToMatrix(const Quaternion& q) {
	Matrix4x3 m;
	const GLfloat xx = 2.0f * q.x * q.x;
	const GLfloat xy = 2.0f * q.x * q.y;
	const GLfloat xz = 2.0f * q.x * q.z;
	const GLfloat xw = 2.0f * q.x * q.w;
	const GLfloat yy = 2.0f * q.y * q.y;
	const GLfloat yz = 2.0f * q.y * q.z;
	const GLfloat yw = 2.0f * q.y * q.w;
	const GLfloat zz = 2.0f * q.z * q.z;
	const GLfloat zw = 2.0f * q.z * q.w;
	m.SetVector(0, Vector4F(1.0f - yy - zz, xy + zw, xz - yw, 0.0f));
	m.SetVector(1, Vector4F(xy - zw, 1.0f - xx - zz, yz + xw, 0.0f));
	m.SetVector(2, Vector4F(xz + yw, yz - xw, 1.0f - xx - yy, 0.0f));
	return m;
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
*/
Quaternion Sleap(const Quaternion& qa, const Quaternion& qb, float t)
{
	const float cosHalfTheta = qa.Dot(qb);
	if (std::abs(cosHalfTheta) >= 1.0f) {
		return qa;
	}
	const float halfTheta = std::acos(cosHalfTheta);
	const float sinHalfTheta = std::sin(halfTheta);
	if ((sinHalfTheta >= 0.0f && sinHalfTheta < 0.001f) || (sinHalfTheta < 0.0f && sinHalfTheta > -0.001f)) {
		return (qa + qb) * 0.5f;
	}
	const float ratioA = std::sin((1.0f - t) * halfTheta) / sinHalfTheta;
	const float ratioB = std::sin(t * halfTheta) / sinHalfTheta;
	return qa * ratioA + qb * ratioB;
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
		result.resize(jointList.size(), { Quaternion(0, 0, 0, 1), Vector3F(0, 0, 0) });
		return result;
	}
	currentTime += t;
	result.reserve(jointList.size());
	for (int i = 0; i < jointList.size(); ++i) {
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
	RotTrans rt = rtBuf * joint->initialPose;
	rt *= parentRT;
	rtBuf = rt * joint->invBindPose;
	if (joint->offChild) {
		UpdateJointRotTrans(rtList, root, joint + joint->offChild, rt);
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
	std::vector<::RotTrans> rtList = animationPlayer.Update(mesh->jointList, t);
	UpdateJointRotTrans(rtList, &mesh->jointList[0], &mesh->jointList[0], ::RotTrans::Unit());
	std::transform(rtList.begin(), rtList.end(), bones.begin(), [this](const ::RotTrans& rt) { return ToMatrix(rt * this->rotTrans); });
  }
}

/** コンストラクタ.
*/
Renderer::Renderer(android_app* s)
  : state(s)
  , isInitialized(false)
  , startTime(0)
  , prevFrames(0)
  , fboMain(0)
  , fboSub(0)
  , fboShadow0(0)
  , fboShadow1(0)
  , fboHDR0(0)
  , fboHDR1(0)
  , depth(0)
{
}

/** デストラクタ.
*/
Renderer::~Renderer()
{
	Unload();
}

/** 描画環境の初期設定.
* OpenGL環境の再構築が必要になった場合、その都度この関数を呼び出す必要がある.
*/
void Renderer::Initialize()
{
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	static const EGLint attribs[] = {
		EGL_BLUE_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 16,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	EGLConfig config;
	EGLint numConfigs;
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	EGLint format;
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(state->window, 0, 0, format);
	surface = eglCreateWindowSurface(display, config, state->window, nullptr);
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

#if 1
#define	LOG_SHADER_INFO(s) { \
		GLint tmp; \
		glGetIntegerv(s, &tmp); \
		LOGI(#s": %d", tmp); }

	GLint formatCount;
	glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &formatCount);
	std::vector<GLint> formatArray;
	formatArray.resize(formatCount);
	glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, &formatArray[0]);
	for (int i = 0; i < formatCount; ++i) {
		LOGI("tc[%02d]=%04x", i, formatArray[i]);
	}
	LOG_SHADER_INFO(GL_MAX_TEXTURE_IMAGE_UNITS);
	LOG_SHADER_INFO(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
#undef LOG_SHADER_INFO
#endif

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);

	static const char* const shaderNameList[] = {
	  "default",
	  "default2D",
	  "skybox",
	  "shadow",
	  "bilinear4x4",
	  "gaussian3x3",
	  "reduce4",
	  "hdrdiff",
	  "applyhdr",
	};
	for (const auto e : shaderNameList) {
		const std::string vert = std::string("Shaders/") + std::string(e) + std::string(".vert");
		const std::string frag = std::string("Shaders/") + std::string(e) + std::string(".frag");
		if (boost::optional<Shader> s = CreateShaderProgram(state, e, vert.c_str(), frag.c_str())) {
			shaderList.insert({ s->id, *s });
		}
	}

	InitTexture();

	{
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 1024 * 10, 0, GL_STATIC_DRAW);
		vboEnd = 0;

		glGenBuffers(1, &ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 1024 * 10 * 3, 0, GL_STATIC_DRAW);
		iboEnd = 0;

		InitMesh();

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

	struct {
		const char* const name;
		GLuint* p;
	} fboList[] = {
		{ "fboSub", &fboSub },
		{ "fboShadow0", &fboShadow0 },
		{ "fboShadow1", &fboShadow1 },
		{ "fboHDR0", &fboHDR0 },
		{ "fboHDR1", &fboHDR1 },
	};
	for (const auto e : fboList) {
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
	isInitialized = true;
}

void Renderer::DrawFont(const Position2F& pos, const char* str)
{
  const Shader& shader = shaderList["default2D"];
  glUseProgram(shader.program);
  glBlendFunc(GL_ONE, GL_ZERO);
  glDisable(GL_CULL_FACE);

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
  glUniform1i(shader.texDiffuse, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textureList["ascii"]->TextureId());
  const Mesh& mesh = meshList["ascii"];
  float x = pos.x;
  for (const char* p = str; *p; ++p) {
	Matrix4x4 mMV = Matrix4x4::Unit();
	mMV = mV * Matrix4x4::Translation(x, pos.y, 0) * mMV;
	glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mMV.f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset + *p * 6 * sizeof(GLushort)));
	x += 16.0f;
  }
  glEnable(GL_CULL_FACE);
}

void Renderer::Render(const Object* begin, const Object* end)
{
	static const int32_t stride = sizeof(Vertex);
	static const void* const offPosition = reinterpret_cast<void*>(offsetof(Vertex, position));
	static const void* const offWeight = reinterpret_cast<void*>(offsetof(Vertex, weight[0]));
	static const void* const offNormal = reinterpret_cast<void*>(offsetof(Vertex, normal));
	static const void* const offBoneID = reinterpret_cast<void*>(offsetof(Vertex, boneID[0]));
	static const void* const offTexCoord01 = reinterpret_cast<void*>(offsetof(Vertex, texCoord[0]));

	if (!isInitialized) {
	  return;
	}
	if (begin == end) {
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

	static const float MAIN_RENDERING_PATH_WIDTH = 480.0f;
	static const float MAIN_RENDERING_PATH_HEIGHT = 800.0f;
	static const float FBO_SUB_WIDTH = 128.0f;
	static const float FBO_SUB_HEIGHT = 128.0f;
	static const float SHADOWMAP_MAIN_WIDTH = 512.0f;
	static const float SHADOWMAP_MAIN_HEIGHT = 512.0f;
	static const float SHADOWMAP_SUB_WIDTH = SHADOWMAP_MAIN_WIDTH * 0.25f;
	static const float SHADOWMAP_SUB_HEIGHT = SHADOWMAP_MAIN_HEIGHT * 0.25f;

	// とりあえず適当なカメラデータからビュー行列を設定.
	Matrix4x4 mView;
	Vector3F eye;
	static Vector3F at(0, 0.5, 0);
	{
	  static float degree = 0;
	  const float r = degreeToRadian(degree);
	  eye = Vector3F(std::cos(r) * -3.0f, 2, std::sin(r) * -3.0f);
	  degree += 0.5f;
	  if (degree >= 360) {
		degree -= 360;
	  }
	  mView = LookAt(eye, at, Vector3F(0, 1, 0));
	}

	// 射影行列を設定.
	static const float near = 0.1f;
	static const float far = 500.0f;
	Matrix4x4 mProj;
	{
	  int32_t viewport[4];
	  glGetIntegerv(GL_VIEWPORT, viewport);
	  mProj = Perspective(60.0f, viewport[2], viewport[3], near, far);
	}

	// shadow path.

	static float fov = 60.0f;
	static Vector3F viewPos(50, 50, 50);
	const Matrix4x4 mViewL = LookAt(viewPos, Vector3F(0, 0, 0), Vector3F(0, 1, 0));
	const Matrix4x4 mProjL = Perspective(fov, SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT, 0.1f, 500);
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
	  const float r = (a - Position2F(0, distance)).Length(); // 視錐台の外接円の半径.

	  const Vector3F vEye = (at - eye).Normalize();
	  const Vector3F frustumCenter = eye + (vEye * distance);
	  Vector4F transformedCenter = mProjL * mViewL * frustumCenter;
	  static float ms = 4.0f;// transformedCenter.w / frustumRadius;
	  static float mss = 2.25f;
	  const float mx = transformedCenter.x / transformedCenter.w;
	  const float my = transformedCenter.y / transformedCenter.w;
	  const Matrix4x4 m = { {
	    ms, 0, 0, 0,
	    0, ms, 0, 0,
	    0, 0,  1, 0,
		-mx * mss, -my * mss, 0, 1,
	  } };
	  mCropL = m;
	}
	const Matrix4x4 mVPForShadow = mCropL * mProjL * mViewL;
#if 1
	{
#if 1
		glEnableVertexAttribArray(VertexAttribLocation_Position);
		glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
		glDisableVertexAttribArray(VertexAttribLocation_Normal);
		glDisableVertexAttribArray(VertexAttribLocation_TexCoord01);
		glEnableVertexAttribArray(VertexAttribLocation_Weight);
		glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
		glEnableVertexAttribArray(VertexAttribLocation_BoneID);
		glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);
#endif
		glBindFramebuffer(GL_FRAMEBUFFER, fboMain);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_ONE, GL_ZERO);
		glCullFace(GL_FRONT);

		glViewport(0, 0, SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT);
		glScissor(0, 0, SHADOWMAP_MAIN_WIDTH, SHADOWMAP_MAIN_HEIGHT);
		glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const Shader& shader = shaderList["shadow"];
		glUseProgram(shader.program);
		glUniformMatrix4fv(shader.matLightForShadow, 1, GL_FALSE, mVPForShadow.f);

		for (const Object* itr = begin; itr != end; ++itr) {
			const Object& obj = *itr;
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
			glDrawElements(GL_TRIANGLES, obj.Mesh()->iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(obj.Mesh()->iboOffset));
		}
		if(1){
		  Matrix4x4 m = Matrix4x4::RotationX(degreeToRadian(90.0f));
		  //m.Set(1, 3, -5.0f);
		  glUniform4fv(shader.bones, 3, m.f); // 平行移動を含まないMatrix4x4はMatrix4x3の代用になりうるけどお行儀悪い.
		  const Mesh& mesh = meshList["board"];
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
		  const Vector3F frustumCenter = eye + (vEye * distance);
		  Matrix4x4 m = Matrix4x4::FromScale(r * 2, r * 2, r * 2);
		  m.Set(0, 3, frustumCenter.x);
		  m.Set(1, 3, frustumCenter.y);
		  m.Set(2, 3, frustumCenter.z);
		  glUniform4fv(shader.bones, 3, m.f);
		  const Mesh& mesh = meshList["Sphere"];
		  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
		}
		glFlush();
	}

	// fboMain ->(bilinear4x4)-> fboShadow0
	{
		glEnableVertexAttribArray(VertexAttribLocation_Position);
		glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
		glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
		glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);

		glBindFramebuffer(GL_FRAMEBUFFER, fboShadow0);
		glViewport(0, 0, SHADOWMAP_SUB_WIDTH, SHADOWMAP_SUB_HEIGHT);
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
		glViewport(0, 0, SHADOWMAP_SUB_WIDTH, SHADOWMAP_SUB_HEIGHT);
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

		glFlush();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
#endif

	// color path.
	glBindFramebuffer(GL_FRAMEBUFFER, fboMain);
	glViewport(0, 0, MAIN_RENDERING_PATH_WIDTH, MAIN_RENDERING_PATH_HEIGHT);
	glScissor(0, 0, MAIN_RENDERING_PATH_WIDTH, MAIN_RENDERING_PATH_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);

	glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#if 1
	glEnableVertexAttribArray(VertexAttribLocation_Position);
	glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
	glEnableVertexAttribArray(VertexAttribLocation_Normal);
	glVertexAttribPointer(VertexAttribLocation_Normal, 3, GL_FLOAT, GL_FALSE, stride, offNormal);
	glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
	glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);
	glEnableVertexAttribArray(VertexAttribLocation_Weight);
	glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
	glEnableVertexAttribArray(VertexAttribLocation_BoneID);
	glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);
#endif
	GLuint currentProgramId = 0;

#if 1
	for (const Object* itr = begin; itr != end; ++itr) {
		const Object& obj = *itr;
		if (!obj.IsValid()) {
			continue;
		}

		const Shader& shader = *itr->Shader();
		if (shader.program != currentProgramId) {
			glUseProgram(shader.program);
			currentProgramId = shader.program;

			glUniform3f(shader.eyePos, eye.x, eye.y, eye.z);
			glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
			glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mView.f);
			glUniformMatrix4fv(shader.matLightForShadow, 1, GL_FALSE, mVPForShadow.f);

			// 適当にライトを置いてみる.
			glUniform3f(shader.lightColor, 7000.0f, 6000.0f, 5000.0f);
			glUniform3f(shader.lightPos, 50, 50, 50);

			glUniform1i(shader.texDiffuse, 0);
			//	glUniform1i(shader.texNormal, 1);
			static const int texIBLId[] = { 1, 2, 3 };
			glUniform1iv(shader.texIBL, 3, texIBLId);
			glUniform1i(shader.texShadow, 4);

			// IBL用テクスチャを設定.
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_high"]->TextureId());
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_low"]->TextureId());
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["irradiance"]->TextureId());
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, textureList["fboShadow1"]->TextureId());
		}

		glUniform4f(shader.materialColor, Fix8ToFloat(obj.Color().r), Fix8ToFloat(obj.Color().g), Fix8ToFloat(obj.Color().b), Fix8ToFloat(obj.Color().a));
		glUniform2f(shader.materialMetallicAndRoughness, Fix8ToFloat(obj.Metallic()), Fix8ToFloat(obj.Roughness()));

		{
			const auto& tex = obj.Mesh()->texDiffuse;
			glActiveTexture(GL_TEXTURE0);
			if (tex) {
				glBindTexture(GL_TEXTURE_2D, tex->TextureId());
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
		}
		glDrawElements(GL_TRIANGLES, obj.Mesh()->iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(obj.Mesh()->iboOffset));
	}
#endif

#if 1
	{
		const Shader& shader = shaderList["default"];
		glUseProgram(shader.program);

		glUniform3f(shader.eyePos, eye.x, eye.y, eye.z);
		glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
//		const Matrix4x4 mV = LookAt(Vector3F(0, 0, -3), Vector3F(0, 0, 0), Vector3F(0, 1, 0));
//		glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mV.f);
		glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mView.f);
		glUniformMatrix4fv(shader.matLightForShadow, 1, GL_FALSE, mVPForShadow.f);

		// 適当にライトを置いてみる.
		glUniform3f(shader.lightColor, 0.0f, 0.0f, 0.0f);
		glUniform3f(shader.lightPos, 50, 50, 50);

		glUniform1i(shader.texDiffuse, 0);
		//	glUniform1i(shader.texNormal, 1);
		static const int texIBLId[] = { 1, 2, 3 };
		glUniform1iv(shader.texIBL, 3, texIBLId);
		glUniform1i(shader.texShadow, 4);

		// IBL用テクスチャを設定.
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_high"]->TextureId());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_low"]->TextureId());
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["irradiance"]->TextureId());
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, textureList["fboShadow1"]->TextureId());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureList["dummy"]->TextureId());
		Matrix4x4 m = Matrix4x4::RotationX(degreeToRadian(90.0f));
		//m.Set(1, 3, -5.0f);
		glUniform4fv(shader.bones, 3, m.f); // 平行移動を含まないMatrix4x4はMatrix4x3の代用になりうるけどお行儀悪い.
		const Mesh& mesh = meshList["board"];
		glUniform4f(shader.materialColor, 1.0f, 1.0f, 1.0f, 1.0f);
		glUniform2f(shader.materialMetallicAndRoughness, 0.0f, 1.0f);
		glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	}
#endif
#if 1
	{
		const Shader& shader = shaderList["skybox"];
		glUseProgram(shader.program);

		glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mProj.f);
		glUniformMatrix4fv(shader.matView, 1, GL_FALSE, mView.f);

		glUniform1i(shader.texDiffuse, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureList["skybox_high"]->TextureId());
		const Mesh& mesh = meshList["skybox"];
		glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));

		glFlush();
	}
#endif

	// hdr path.

	// fboMain ->(reduce4)->fboSub
	{
	  glBindFramebuffer(GL_FRAMEBUFFER, fboSub);
	  glViewport(0, 0, MAIN_RENDERING_PATH_WIDTH / 4, MAIN_RENDERING_PATH_HEIGHT / 4);
	  glDisable(GL_DEPTH_TEST);
	  glBlendFunc(GL_ONE, GL_ZERO);
	  glCullFace(GL_BACK);

	  const Shader& shader = shaderList["reduce4"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glUniform4f(shader.unitTexCoord, 480.0f / 512.0f, 800.0f / 800.0f, 1.0f / 512.0f, 1.0f / 800.0f);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboMain"]->TextureId());

	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));

	  glFlush();
	}
	// fboSub ->(hdrdiff)->fboHDR0
	{
	  glBindFramebuffer(GL_FRAMEBUFFER, fboHDR0);
	  glViewport(0, 0, MAIN_RENDERING_PATH_WIDTH / 16, MAIN_RENDERING_PATH_HEIGHT / 16);

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

	  glFlush();
	}
	// fboHDR0 ->(reduce4)->fboHDR1
	{
	  glBindFramebuffer(GL_FRAMEBUFFER, fboHDR1);
	  glViewport(0, 0, 8, 13);
	  const Shader& shader = shaderList["reduce4"];
	  glUseProgram(shader.program);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  glUniform1i(shader.texDiffuse, 0);
	  glUniform4f(shader.unitTexCoord, 1.0f, 1.0f, 1.0f / 30.0f, 1.0f / 50.0f);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboHDR0"]->TextureId());
	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));

	  glFlush();
	}

	// final path.
	{
	  glBindFramebuffer(GL_FRAMEBUFFER, 0);
	  glViewport(0, 0, width, height);
	  glScissor(0, 0,width, height);
	  glDisable(GL_DEPTH_TEST);
	  glDepthFunc(GL_ALWAYS);
	  glCullFace(GL_BACK);
#if 1
	  glEnableVertexAttribArray(VertexAttribLocation_Position);
	  glVertexAttribPointer(VertexAttribLocation_Position, 3, GL_FLOAT, GL_FALSE, stride, offPosition);
	  glEnableVertexAttribArray(VertexAttribLocation_Normal);
	  glVertexAttribPointer(VertexAttribLocation_Normal, 3, GL_FLOAT, GL_FALSE, stride, offNormal);
	  glEnableVertexAttribArray(VertexAttribLocation_TexCoord01);
	  glVertexAttribPointer(VertexAttribLocation_TexCoord01, 4, GL_UNSIGNED_SHORT, GL_FALSE, stride, offTexCoord01);
	  glEnableVertexAttribArray(VertexAttribLocation_Weight);
	  glVertexAttribPointer(VertexAttribLocation_Weight, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offWeight);
	  glEnableVertexAttribArray(VertexAttribLocation_BoneID);
	  glVertexAttribPointer(VertexAttribLocation_BoneID, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offBoneID);
#endif
	  const Shader& shader = shaderList["applyhdr"];
	  glUseProgram(shader.program);
	  glBlendFunc(GL_ONE, GL_ZERO);

	  Matrix4x4 mtx = Matrix4x4::Unit();
	  mtx.Scale(1.0f, -1.0f, 1.0f);
	  glUniformMatrix4fv(shader.matProjection, 1, GL_FALSE, mtx.f);

	  static const int texSource[] = { 0, 1, 2, 3 };
	  glUniform1iv(shader.texSource, 4, texSource);
	  glActiveTexture(GL_TEXTURE0);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboMain"]->TextureId());
	  glActiveTexture(GL_TEXTURE1);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboSub"]->TextureId());
	  glActiveTexture(GL_TEXTURE2);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboHDR0"]->TextureId());
	  glActiveTexture(GL_TEXTURE3);
	  glBindTexture(GL_TEXTURE_2D, textureList["fboHDR1"]->TextureId());
	  const Mesh& mesh = meshList["board2D"];
	  glDrawElements(GL_TRIANGLES, mesh.iboSize, GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid*>(mesh.iboOffset));
	}
	{
	  ++frames;
	  timespec tmp;
	  const int err = clock_gettime(CLOCK_MONOTONIC, &tmp);
	  if (err < 0) {
		LOGI("ERROR(%d) from; clock_gettime(CLOCK_MONOTONIC)", err);
	  } else {
		const uint64_t curTime = tmp.tv_sec * (1000UL * 1000UL * 1000UL) + tmp.tv_nsec;
		if (curTime - startTime >= (1000UL * 1000UL * 1000UL)) {
		  prevFrames = frames;
		  startTime = curTime;
		  frames = 0;
		}
	  }
	  std::string s("FPS:");
	  s += boost::lexical_cast<std::string>(prevFrames);
	  DrawFont(Position2F(16, 56), s.c_str());
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

	// テクスチャのバインドを解除.
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	// バッファオブジェクトのバインドを解除.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::Update(float dTime)
{
}

void Renderer::Unload()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	GLuint* fboPtrList[] = {
	  &fboMain, &fboSub, &fboShadow0, &fboShadow1, &fboHDR0, &fboHDR1, &depth
	};
	for (auto e : fboPtrList) {
	  if (*e) {
		glDeleteRenderbuffers(1, e);
		*e = 0;
	  }
	}

	animationList.clear();
	meshList.clear();
	textureList.clear();

	for (auto&& e : shaderList) {
		glDeleteProgram(e.second.program);
	}
	shaderList.clear();

	if (vbo) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (ibo) {
		glDeleteBuffers(1, &ibo);
		ibo = 0;
	}

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
	eglSwapBuffers(display, surface);
}

namespace {
	template<typename T, typename F>
	std::vector<T> split(const std::string& str, char delim, F func) {
		std::vector<T> v;
		std::istringstream ss(str);
		std::string item;
		while (std::getline(ss, item, delim)) {
			v.push_back(func(item));
		}
		return v;
	}
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
}

/** 描画に必要なポリゴンメッシュの初期化.
* ポリゴンメッシュを読み込み、バッファオブジェクトに格納する.
* Initialize()から呼び出されるため、明示的に呼び出す必要はない.
*/
void Renderer::InitMesh()
{
	LoadMesh("Sphere.dae", "dummy");
	LoadMesh("cubes.dae", "wood");
	CreateSkyboxMesh();
	CreateBoardMesh("board", Vector3F(25.0f, 25.0f, 1.0f));
	CreateBoardMesh("board2D", Vector3F(1.0f, 1.0f, 1.0f));
	CreateAsciiMesh("ascii");
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
		v.position = Position3F(cubeVertices[i + 0], cubeVertices[i + 1], cubeVertices[i + 2]) * 100.0f;
		vertecies.push_back(v);
	}
	static const GLushort cubeIndices[] = {
		2, 1, 0, 0, 3, 2,
		6, 2, 3, 3, 7 ,6,
		5, 6, 7, 7, 4, 5,
		1, 5, 4, 4, 0, 1,
		7, 3, 0, 0, 4, 7,
		1, 2, 6, 6, 5, 1,
	};
	const GLushort offset = vboEnd / sizeof(Vertex);
	for (auto e : cubeIndices) {
		indices.push_back(e + offset);
	}

	Mesh mesh = Mesh("skybox", iboEnd, 6 * 2 * 3);
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
		Vertex v;
		v.position = Position3F(cubeVertices[i + 0], cubeVertices[i + 1], cubeVertices[i + 2]) * scale;
		v.weight[0] = 255;
		v.weight[1] = v.weight[2] = v.weight[3] = 0;
		v.normal = Vector3F(0.0f, 0.0f, 1.0f);
		v.boneID[0] = v.boneID[1] = v.boneID[2] = v.boneID[3] = 0;
		v.texCoord[0] = Position2S::FromFloat(cubeVertices[i + 3], cubeVertices[i + 4]);
		v.texCoord[1] = v.texCoord[0];
		vertecies.push_back(v);
	}
	static const GLushort cubeIndices[] = {
		0, 1, 2, 2, 3, 0,
		2, 1, 0, 0, 3, 2,
	};
	const GLushort offset = vboEnd / sizeof(Vertex);
	for (auto e : cubeIndices) {
		indices.push_back(e + offset);
	}

	const Mesh mesh = Mesh(id, iboEnd, 2 * 2 * 3);
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
		Vertex v;
		v.position = Position3F(rectVertecies[i + 0], rectVertecies[i + 1], 0.0f);
		v.weight[0] = 255;
		v.weight[1] = v.weight[2] = v.weight[3] = 0;
		v.normal = Vector3F(0.0f, 0.0f, 1.0f);
		v.boneID[0] = v.boneID[1] = v.boneID[2] = v.boneID[3] = 0;
		v.texCoord[0] = Position2S::FromFloat(rectVertecies[i + 2] + x * 32.0f / 512.0f, rectVertecies[i + 3] + (1.0f - (y + 1) * 64.0f / 512.0f));
		v.texCoord[1] = v.texCoord[0];
		vertecies.push_back(v);
	  }
	}
  }

  std::vector<GLushort> indices;
  indices.reserve(8 * 16 * 6);
  static const GLushort rectIndices[] = {
	0, 1, 2, 2, 3, 0,
  };
  const GLushort offset = vboEnd / sizeof(Vertex);
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

void Renderer::LoadMesh(const char* filename, const char* texName)
{
	if (auto pBuf = LoadFile(state, filename)) {
		// property_treeを使ってCOLLADAファイルを解析.
		RawBufferType& buf = *pBuf;
		struct membuf : public std::streambuf {
			explicit membuf(char* b, char* e) { this->setg(b, b, e); }
		};
		membuf mb((char*)&buf[0], (char*)&buf[buf.size()]);
		std::istream is(&mb);
		BPT::ptree collada;
		try {
			BPT::read_xml(is, collada);
			LOGI("XML READ SUCCESS");
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
			for (auto& e : scenesNode) {
				if (e.first != "node" || e.second.get<std::string>("<xmlattr>.type") != "NODE") {
					continue;
				}
				const BPT::ptree& nodeElem = e.second;
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
						int offset = 0;
						for (auto& e : vcount) {
							std::array<int, 4> boneId = { { 0, 0, 0, 0 } };
							std::array<float, 4> weight = { { 0, 0, 0, 0 } };
							for (int i = 0; i < 4; ++i) {
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
				const BPT::ptree& trianglesNode = meshNode.get_child("triangles");
				LOGI("%s", trianglesNode.get<std::string>("<xmlattr>.material").c_str());

				// input要素を解析し、対応するソース及びオフセットを得る.
				std::vector<Position3F> posArray;
				std::vector<Vector3F> normalArray;
				std::vector<Position2S> texcoordArray[VERTEX_TEXTURE_COUNT_MAX];
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
						const std::vector<float> tmp = split<float>(getSourceArray(meshNode, srcId), ' ', [](const std::string& s) -> float { return atof(s.c_str()); });
						posArray.shrink_to_fit();
						posArray.reserve(tmp.size() / 3);
						for (size_t i = 0; i + 2 < tmp.size(); i += 3) {
							posArray.push_back(Position3F(tmp[i], tmp[i + 1], tmp[i + 2]));
						}
						LOGI("POS ARRAY SIZE:%d", posArray.size());
						vertexOff = e.second.get<int>("<xmlattr>.offset");
						offsetMax = std::max(offsetMax, vertexOff);
					}
					else if (semantic == "NORMAL") {
						LOGI("NORMAL FOUND");
						const std::string srcId = e.second.get<std::string>("<xmlattr>.source").substr(1);
						LOGI("NORMAL SOURCE: #%s", srcId.c_str());
						const std::vector<float> tmp = split<float>(getSourceArray(meshNode, srcId), ' ', [](const std::string& s) -> float { return atof(s.c_str()); });
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
							const std::vector<float> tmp = split<float>(getSourceArray(meshNode, srcId), ' ', [](const std::string& s) -> float { return atof(s.c_str()); });
							texcoordArray[index].shrink_to_fit();
							texcoordArray[index].reserve(tmp.size() / 3);
							for (size_t i = 0; i + 1 < tmp.size(); i += 2) {
								texcoordArray[index].push_back(Position2S::FromFloat(tmp[i], tmp[i + 1]));
							}
							LOGI("TEXCOORD[%d] ARRAY SIZE:%d", index, texcoordArray[index].size());
							texcoordOff[index] = e.second.get<int>("<xmlattr>.offset");
							offsetMax = std::max(offsetMax, texcoordOff[index]);
						}
					}
				}
				const int stride = offsetMax + 1; // 頂点ごとに必要とするインデックスの数.
				LOGI("STRIDE:%d", stride);

				const int triCount = trianglesNode.get<int>("<xmlattr>.count"); // 三角形(trianglesなので)の個数. インデックスの要素数ではないことに注意.
				std::vector<uint_fast64_t> vertIdList;
				vertIdList.reserve(triCount * 3);
				const std::vector<int> indexList = split<int>(trianglesNode.get<std::string>("p"), ' ', [](const std::string& s)->int { return atoi(s.c_str()); });
				for (int i = 0; i < indexList.size(); i += stride) {
					const uint_fast64_t vertId =
						(static_cast<uint_fast64_t>((vertexOff != -1) ? indexList[i + vertexOff] : 0xfff) << 0) |
						(static_cast<uint_fast64_t>((normalOff != -1) ? indexList[i + normalOff] : 0xfff) << 12) |
						(static_cast<uint_fast64_t>((texcoordOff[0] != -1) ? indexList[i + texcoordOff[0]] : 0xfff) << 24) |
						(static_cast<uint_fast64_t>((texcoordOff[1] != -1) ? indexList[i + texcoordOff[1]] : 0xfff) << 36);
					// LOGI("vertId: %llx", vertId);
					auto itr = std::find(vertIdList.begin(), vertIdList.end(), vertId);
					if (itr != vertIdList.end()) {
						indices.push_back(itr - vertIdList.begin() + iboEnd / sizeof(GLushort));
					}
					else {
						Vertex v;
						const int posId = indexList[i + vertexOff];
						v.position = (vertexOff != -1) ? posArray[posId] : Position3F();
						v.normal = (normalOff != -1) ? normalArray[indexList[i + normalOff]] : Vector3F();
						for (int j = 0; j < VERTEX_TEXTURE_COUNT_MAX; ++j) {
							v.texCoord[j] = (texcoordOff[j] != -1) ? texcoordArray[j][indexList[i + texcoordOff[j]]] : Position2S();
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
						indices.push_back(vertIdList.size() + iboEnd / sizeof(GLushort));
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

				Mesh mesh = Mesh(nodeElem.get<std::string>("<xmlattr>.id"), baseIndexOffset, triCount * 3);
				if (!boneNameList.empty()) {
					std::vector<Joint> joints;
					joints.resize(boneNameList.size());
					for (int i = 0; i < joints.size(); ++i) {
						skinInfo.invBindPoseMatrixList[i].Decompose(&joints[i].invBindPose.rot, nullptr, &joints[i].invBindPose.trans);
					}
					local::AddJoint(joints, scenesNode, boneNameList);
					mesh.SetJoint(boneNameList, joints);
				}
				if (texName) {
					const auto itr = textureList.find(texName);
					if (itr != textureList.end()) {
						mesh.texDiffuse = itr->second;
					}
				}
				meshList.insert({ mesh.id, mesh });
				baseIndexOffset += sizeof(GLushort) * triCount * 3;
			}
			LOGI("MAKING VERTEX LIST SUCCESS:%d, %d", vertecies.size(), indices.size());
			glBufferSubData(GL_ARRAY_BUFFER, vboEnd, vertecies.size() * sizeof(Vertex), &vertecies[0]);
			vboEnd += vertecies.size() * sizeof(Vertex);
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, iboEnd, indices.size() * sizeof(GLushort), &indices[0]);
			iboEnd += indices.size() * sizeof(GLushort);
		}
		catch (BPT::ptree_bad_path& e) {
			LOGI("BAD PATH");
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
						for (int i = 0; i < timeList.size(); ++i) {
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
				LOGI("BAD PATH");
			}
		}
	}
}
void Renderer::InitTexture()
{
	textureList.insert({ "fboMain", Texture::CreateEmpty2D(512, 800) }); // 影初期描画、カラー描画
	textureList.insert({ "fboSub", Texture::CreateEmpty2D(120, 200) }); // カラー & HDR(120x200)
	textureList.insert({ "fboShadow0", Texture::CreateEmpty2D(128, 128) }); // 影縮小
	textureList.insert({ "fboShadow1", Texture::CreateEmpty2D(128, 128) }); // 影ぼかし
	textureList.insert({ "fboHDR0", Texture::CreateEmpty2D(30, 50) }); // HDR(30x50)
	textureList.insert({ "fboHDR1", Texture::CreateEmpty2D(8, 13) }); // HDR(8x13)

	textureList.insert({ "dummyCubeMap", Texture::CreateDummyCubeMap() });
	textureList.insert({ "dummy", Texture::CreateDummy2D() });
	textureList.insert({ "ascii", Texture::LoadKTX(state, "Textures/ascii.ktx") });

	textureList.insert({ "skybox_high", Texture::LoadKTX(state, "skybox_high.ktx") });
	textureList.insert({ "skybox_low", Texture::LoadKTX(state, "skybox_low.ktx") });
	textureList.insert({ "irradiance", Texture::LoadKTX(state, "irradiance.ktx") });
	textureList.insert({ "wood", Texture::LoadKTX(state, "wood.ktx") });
}

Object Renderer::CreateObject(const char* meshName, const Material& m, const char* shaderName)
{
	return Object(RotTrans::Unit(), &meshList.find(meshName)->second, m, &shaderList[shaderName]);
}

const Animation* Renderer::GetAnimation(const char* name)
{
	return &animationList[name];
}
