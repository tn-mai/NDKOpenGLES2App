#ifndef MT_RENDERER_H_INCLUDED
#define MT_RENDERER_H_INCLUDED

#include "texture.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <vector>
#include <map>
#include <string>
#include <math.h>
#include <float.h>

struct Matrix4x3;
struct Matrix4x4;
struct Vector2F;
struct Vector3F;

/**
* 3D座標.
*/
struct Position3F {
	GLfloat x, y, z;
	Position3F() {}
	constexpr Position3F(GLfloat a, GLfloat b, GLfloat c) : x(a), y(b), z(c) {}
	bool operator==(const Position3F& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const Position3F& rhs) const { return !(*this == rhs); }
	Position3F& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	Position3F operator*(GLfloat rhs) const { return Position3F(*this) *= rhs; }
	Position3F& operator+=(const Vector3F& rhs);
	Position3F operator+(const Vector3F& rhs) const;
	Position3F& operator-=(const Vector3F& rhs);
	Position3F operator-(const Vector3F& rhs) const;
	Position3F& operator*=(const Vector3F& rhs);
	Position3F operator*(const Vector3F& rhs) const;
	Position3F& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }
	Position3F operator/(GLfloat rhs) const { return Position3F(*this) /= rhs; }
};

/**
* 2D座標.
*/
struct Position2F {
	GLfloat x, y;
	Position2F() {}
	constexpr Position2F(GLfloat a, GLfloat b) : x(a), y(b) {}
	bool operator==(const Position2F& rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Position2F& rhs) const { return !(*this == rhs); }
	Position2F& operator-=(const Vector2F&);
	Position2F& operator+=(const Vector2F&);
};

/**
* 2D座標(short).
*/
struct Position2S {
	GLushort x, y;
	Position2S() {}
	constexpr Position2S(GLushort a, GLushort b) : x(a), y(b) {}
	bool operator==(const Position2S& rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Position2S& rhs) const { return !(*this == rhs); }
	static constexpr Position2S FromFloat(float a, float b) {
		return Position2S(a * static_cast<float>(0xffff), b * static_cast<float>(0xffff));
	}
	Position2F ToFloat() const { return Position2F(static_cast<float>(x) / 0xffff, static_cast<float>(y) / 0xffff); }
};

/**
* ベクトル
*/
struct Vector3S {
	GLshort x, y, z;
	Vector3S() {}
	constexpr Vector3S(GLshort a, GLshort b, GLshort c) : x(a), y(b), z(c) {}
};

struct Vector2F {
  GLfloat x, y;
  Vector2F() {}
  constexpr Vector2F(GLfloat a, GLfloat b) : x(a), y(b) {}
  bool operator==(const Vector2F& rhs) const { return x == rhs.x && y == rhs.y; }
  bool operator!=(const Vector2F& rhs) const { return !(*this == rhs); }
  Vector2F& operator+=(const Vector2F& rhs) { x += rhs.x; y += rhs.y; return *this; }
  Vector2F operator+(const Vector2F& rhs) const { return Vector2F(*this) += rhs; }
  Vector2F& operator-=(const Vector2F& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
  Vector2F operator-(const Vector2F& rhs) const { return Vector2F(*this) -= rhs; }
  Vector2F& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; return *this; }
  Vector2F operator*(GLfloat rhs) const { return Vector2F(*this) *= rhs; }
  friend Vector2F operator*(GLfloat lhs, const Vector2F& rhs) { return rhs * lhs; }
  Vector2F& operator*=(const Vector2F& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
  Vector2F operator*(const Vector2F& rhs) const { return Vector2F(*this) *= rhs; }
  Vector2F& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; return *this; }
  Vector2F operator/(GLfloat rhs) const { return Vector2F(*this) /= rhs; }
  Vector2F& operator/=(const Vector2F& rhs) { x /= rhs.x; y /= rhs.y; return *this; }
  Vector2F operator/(const Vector2F& rhs) const { return Vector2F(*this) /= rhs; }
  GLfloat Length() const { return sqrtf(x * x + y * y); }
  Vector2F& Normalize() { return operator/=(Length()); }
  GLfloat Cross(const Vector2F& rhs) const { return x * rhs.y - y * rhs.x; }
  GLfloat Dot(const Vector2F& rhs) const { return x * rhs.x + y * rhs.y; }
  Position2F ToPos() const { return Position2F(x, y); }
  static constexpr Vector2F Unit() { return Vector2F(0, 0); }
};

struct Vector3F {
	GLfloat x, y, z;
	Vector3F() {}
	constexpr Vector3F(GLfloat a, GLfloat b, GLfloat c) : x(a), y(b), z(c) {}
	bool operator==(const Vector3F& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const Vector3F& rhs) const { return !(*this == rhs); }
	Vector3F operator-() const { return Vector3F(-x, -y, -z); }
	Vector3F& operator+=(const Vector3F& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	Vector3F operator+(const Vector3F& rhs) const { return Vector3F(*this) += rhs; }
	Vector3F& operator-=(const Vector3F& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	Vector3F operator-(const Vector3F& rhs) const { return Vector3F(*this) -= rhs; }
	Vector3F& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	Vector3F operator*(GLfloat rhs) const { return Vector3F(*this) *= rhs; }
	friend Vector3F operator*(GLfloat lhs, const Vector3F& rhs) { return rhs * lhs; }
	Vector3F& operator*=(const Vector3F& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
	Vector3F operator*(const Vector3F& rhs) const { return Vector3F(*this) *= rhs; }
	Vector3F& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }
	Vector3F operator/(GLfloat rhs) const { return Vector3F(*this) /= rhs; }
	GLfloat Length() const { return sqrtf(x * x + y * y + z * z); }
	Vector3F& Normalize() {
	  const float l = Length();
	  if (l > FLT_EPSILON) {
		return operator/=(l);
	  }
	  return *this;
	}
	Vector3F Cross(const Vector3F& rhs) const { return Vector3F(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x); }
	GLfloat Dot(const Vector3F& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
	static constexpr Vector3F Unit() { return Vector3F(0, 0, 0); }
};

struct Vector4F {
	GLfloat x, y, z, w;
	Vector4F() {}
	constexpr Vector4F(const Vector3F& v, GLfloat d = 1.0f) : x(v.x), y(v.y), z(v.z), w(d) {}
	constexpr Vector4F(GLfloat a, GLfloat b, GLfloat c, GLfloat d) : x(a), y(b), z(c), w(d) {}
	bool operator==(const Vector4F& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=(const Vector4F& rhs) const { return !(*this == rhs); }
	Vector4F& operator+=(const Vector4F& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
	Vector4F operator+(const Vector4F& rhs) const { return Vector4F(*this) += rhs; }
	Vector4F& operator-=(const Vector4F& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w;  return *this; }
	Vector4F operator-(const Vector4F& rhs) const { return Vector4F(*this) -= rhs; }
	Vector4F& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; z *= rhs; w *= rhs;  return *this; }
	Vector4F operator*(GLfloat rhs) const { return Vector4F(*this) *= rhs; }
	friend Vector4F operator*(GLfloat lhs, const Vector4F& rhs) { return rhs * lhs; }
	Vector4F& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; z /= rhs; w /= rhs;  return *this; }
	Vector4F operator/(GLfloat rhs) const { return Vector4F(*this) /= rhs; }
	GLfloat Length() const { return sqrtf(x * x + y * y + z * z + w * w); }
	Vector4F& Normalize() {
	  const float l = Length();
	  if (l > FLT_EPSILON) {
		return operator/=(l);
	  }
	  return *this;
	}
	Vector4F Cross(const Vector4F& rhs) const { return Vector4F(y * rhs.z - z * rhs.y, z * rhs.w - w * rhs.z, w * rhs.x - x * rhs.w, x * rhs.y - y * rhs.x); }
	GLfloat Dot(const Vector4F& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
	GLfloat Dot3(const Vector4F& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
	Vector4F& operator*=(const Matrix4x4& rhs);
	Vector3F ToVec3() const { return Vector3F(x, y, z); }
	static constexpr Vector4F Unit() { return Vector4F(0, 0, 0, 1); }
};

struct Color4B {
	GLubyte r, g, b, a;
	Color4B() {}
	constexpr Color4B(GLubyte rr, GLubyte gg, GLubyte bb, GLubyte aa = 255) : r(rr), g(gg), b(bb), a(aa) {}
	Color4B& operator*=(float n) { r *= n; g *= n; b *= n; a *= n; return *this; }
	Color4B& operator/=(float n) { r /= n; g /= n; b /= n; a /= n; return *this; }
	Color4B& operator*=(Color4B rhs) {
		const int nr = r * rhs.r;
		const int ng = g * rhs.g;
		const int nb = b * rhs.b;
		const int na = a * rhs.a;
		r = nr / 255U;
		g = ng / 255U;
		b = nb / 255U;
		a = na / 255U;
		return *this;
	}
	friend Color4B operator*(Color4B lhs, Color4B rhs) { return Color4B(lhs) *= rhs; }
};

struct Quaternion {
	GLfloat x, y, z, w;
	Quaternion() {}
	constexpr Quaternion(GLfloat a, GLfloat b, GLfloat c, GLfloat d) : x(a), y(b), z(c), w(d) {}
	Quaternion(const Vector3F& axis, GLfloat angle) {
		const float d = sin(angle * 0.5f);
		x = axis.x * d;
		y = axis.y * d;
		z = axis.z * d;
		w = cos(angle * 0.5f);
	}
	Quaternion(const Vector3F& u, const Vector3F& v) {
	  const Vector3F a = u.Cross(v);
	  x = a.x;
	  y = a.y;
	  z = a.z;
	  w = u.Dot(v);
	  w += Length();
	  Normalize();
	}
	bool operator==(const Quaternion& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	bool operator!=(const Quaternion& rhs) const { return !(*this == rhs); }
	Quaternion& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; z *= rhs; w *= rhs;  return *this; }
	Quaternion operator*(GLfloat rhs) const { return Quaternion(*this) *= rhs; }
	friend Quaternion operator*(GLfloat lhs, const Quaternion& rhs) { return rhs * lhs; }
	Quaternion& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; z /= rhs; w /= rhs;  return *this; }
	Quaternion operator/(GLfloat rhs) const { return Quaternion(*this) /= rhs; }
	Quaternion& operator+=(const Quaternion& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w;  return *this; }
	Quaternion operator+(const Quaternion& rhs) const { return Quaternion(*this) += rhs; }
	Quaternion& operator*=(const Quaternion& rhs) { *this = *this * rhs; return *this; }
	Quaternion operator*(const Quaternion& rhs) const {
		return Quaternion(
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
			w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.z,
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
	}
	GLfloat Dot(const Quaternion& q) const { return x * q.x + y * q.y + z * q.z + w * q.w; }
	GLfloat Length() const { return sqrtf(x * x + y * y + z * z + w * w); }
	Quaternion& Normalize() {
	  const float l = Length();
	  if (l > FLT_EPSILON) {
		return operator/=(l);
	  }
	  return *this;
	}
	Quaternion Conjugate() const { return Quaternion(-x, -y, -z, w); }
	Vector3F Apply(const Vector3F& v) const {

		const Quaternion q(
			v.x * w + v.z * y - v.y * z,
			v.y * w + v.x * z - v.z * x,
			v.z * w + v.y * x - v.x * y,
			v.x * x + v.y * y + v.z * z);
		return Vector3F(
			w * q.x + x * q.w + y * q.z - z * q.y,
			w * q.y + y * q.w + z * q.x - x * q.z,
			w * q.z + z * q.w + x * q.y - y * q.x) * (1.0f / Length());
//		const Vector3F xyz(x, y, z);
//		return v + 2.0f * (v.Cross(xyz) + w * v).Cross(xyz);
//		const Vector3F t = xyz.Cross(v) * 2.0f;
//		return v + t * w + xyz.Cross(t);
	}
	friend Quaternion Sleap(const Quaternion& qa, const Quaternion& qb, float t);
	static constexpr Quaternion Unit() { return Quaternion(0, 0, 0, 1); }
};
Quaternion Sleap(const Quaternion& qa, const Quaternion& qb, float t);
Matrix4x3 ToMatrix(const Quaternion& q);

struct Matrix4x4 {
	GLfloat f[4 * 4];

	void SetVector(int n, const Vector4F& v) {
		f[n * 4 + 0] = v.x;
		f[n * 4 + 1] = v.y;
		f[n * 4 + 2] = v.z;
		f[n * 4 + 3] = v.w;
	}
	Vector4F GetVector(int n) const { return Vector4F(f[n * 4 + 0], f[n * 4 + 1], f[n * 4 + 2], f[n * 4 + 3]); }
	Vector4F GetRawVector(int n) const { return Vector4F(f[0 + n], f[4 + n], f[8 + n], f[12 + n]); }
	GLfloat At(int raw, int col) const { return f[raw * 4 + col]; }
	void Set(int raw, int col, GLfloat value) { f[raw * 4 + col] = value; }
	Matrix4x4& Identify() {
		*this = Unit();
		return *this;
	}
	Matrix4x4& Scale(float x, float y, float z) {
		SetVector(0, GetVector(0) * x);
		SetVector(1, GetVector(1) * y);
		SetVector(2, GetVector(2) * z);
		return *this;
	}
	Matrix4x4& Transpose() {
		std::swap(f[1], f[4]);
		std::swap(f[2], f[8]);
		std::swap(f[3], f[12]);
		std::swap(f[6], f[9]);
		std::swap(f[7], f[13]);
		std::swap(f[11], f[14]);
		return *this;
	}
	Matrix4x4& Inverse();
	static Matrix4x4 Translation(float x, float y, float z) {
		Matrix4x4 m = Unit();
		m.Set(3, 0, x);
		m.Set(3, 1, y);
		m.Set(3, 2, z);
		return m;
	}
	static Matrix4x4 FromScale(float x, float y, float z) {
	  Matrix4x4 m;
	  m.SetVector(0, Vector4F(x, 0, 0, 0));
	  m.SetVector(1, Vector4F(0, y, 0, 0));
	  m.SetVector(2, Vector4F(0, 0, z, 0));
	  m.SetVector(3, Vector4F(0, 0, 0, 1));
	  return m;
	}
	static Matrix4x4 RotationX(float angle/* radian */) {
	  const float c = cos(angle);
	  const float s = sin(angle);
	  Matrix4x4 m;
	  m.SetVector(0, Vector4F(1, 0, 0, 0));
	  m.SetVector(1, Vector4F(0, c, -s, 0));
	  m.SetVector(2, Vector4F(0, s, c, 0));
	  m.SetVector(3, Vector4F(0, 0, 0, 1));
	  return m;
	}
	Matrix4x4& operator*=(const Matrix4x4& rhs) {
		const Matrix4x4 m = Matrix4x4(*this).Transpose();
		for (int i = 0; i < 4; ++i) {
			const Vector4F v = rhs.GetVector(i);
			SetVector(i, Vector4F(m.GetVector(0).Dot(v), m.GetVector(1).Dot(v), m.GetVector(2).Dot(v), m.GetVector(3).Dot(v)));
		}
		return *this;
	}
	Matrix4x4 operator*(const Matrix4x4& rhs) const { return Matrix4x4(*this) *= rhs; }
	void Decompose(Quaternion* q, Vector3F* scale, Vector3F* trans) const;
	static const Matrix4x4& Unit() {
		static const Matrix4x4 m = { { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 } };
		return m;
	}
};

/**
  3x3行列 + 平行移動.
  平行移動要素が各行のwに格納されていることに注意. Matrix4x4へ変換するにはwを4行目に移動させる必要がある.
  @todo 転置行列として定義しなおすこと.
*/
struct Matrix4x3 {
	GLfloat f[4 * 3];

	void SetVector(int n, const Vector4F& v) {
		f[n * 4 + 0] = v.x;
		f[n * 4 + 1] = v.y;
		f[n * 4 + 2] = v.z;
		f[n * 4 + 3] = v.w;
	}
	Vector4F GetVector(int n) const { return Vector4F(f[n * 4 + 0], f[n * 4 + 1], f[n * 4 + 2], f[n * 4 + 3]); }
	GLfloat At(int raw, int col) const { return f[raw * 4 + col]; }
	void Set(int raw, int col, GLfloat value) { f[raw * 4 + col] = value; }
	Matrix4x3& operator*=(const Matrix4x3& rhs) {
		Matrix4x4 m;
		m.SetVector(0, Vector4F(f[0], f[4], f[8], 0));
		m.SetVector(1, Vector4F(f[1], f[5], f[9], 0));
		m.SetVector(2, Vector4F(f[2], f[6], f[10], 0));
		m.SetVector(3, Vector4F(f[3], f[7], f[11], 1));
		for (int i = 0; i < 3; ++i) {
			const Vector4F v = rhs.GetVector(i);
			SetVector(i, Vector4F(m.GetVector(0).Dot(v), m.GetVector(1).Dot(v), m.GetVector(2).Dot(v), m.GetVector(3).Dot(v)));
		}
		return *this;
	}
	Matrix4x3 operator*(const Matrix4x3& rhs) const { return Matrix4x3(*this) *= rhs; }
	static const Matrix4x3& Unit() {
		static const Matrix4x3 m = { { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0 } };
		return m;
	}
};

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

/**
* モデルの材質.
*/
struct Material {
	Color4B color;
	GLubyte metallic;
	GLubyte roughness;
	Material() {}
	constexpr Material(Color4B c, GLubyte m, GLubyte r) : color(c), metallic(m), roughness(r) {}
};

struct RotTrans {
	Quaternion rot;
	Vector3F trans;
	RotTrans& operator*=(const RotTrans& rhs) {
		const Vector3F v = rhs.rot.Apply(trans);
		trans = v + rhs.trans;
		rot *= rhs.rot;
		return *this;
	}
	friend RotTrans operator*(const RotTrans& lhs, const RotTrans& rhs) { return RotTrans(lhs) *= rhs; }
	friend RotTrans Interporation(const RotTrans& lhs, const RotTrans& rhs, float t) {
		return { Sleap(lhs.rot, rhs.rot, t), lhs.trans * (1.0f - t) + rhs.trans * t };
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
*/
struct Joint {
	RotTrans invBindPose;
	RotTrans initialPose;
	int offChild;
	int offSibling;
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
	std::vector<ElementList> data;

	ElementPair GetElementByTime(int index, float t) const;
};

struct AnimationPlayer {
	AnimationPlayer() : currentTime(0), pAnime(nullptr), targetList() {}
	void SetAnimation(const Animation* p) { pAnime = p; currentTime = 0.0f; }
	void SetCurrentTime(float t) { currentTime = t; }
	float GetCurrentTime() const { return currentTime; }
	std::vector<RotTrans> Update(const JointList& jointList, float t);

	float currentTime;
	const Animation* pAnime;
	std::vector<const Animation::Element*> targetList;
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
	Texture::TexturePtr texRoughnessAndMetallic;
};

/**
* シェーダ制御情報.
*/
struct Shader
{
	Shader() : program(0) {}
	~Shader();

	GLuint program;

	GLint eyePos;
	GLint lightColor;
	GLint lightPos;
	GLint materialColor;
	GLint materialMetallicAndRoughness;

	GLint texDiffuse;
	GLint texNormal;
	GLint texMetalRoughness;
	GLint texIBL;
	GLint texShadow;
	GLint texSource;

	GLint unitTexCoord;

	GLint matView;
	GLint matProjection;
	GLint matLightForShadow;
	GLint bones;

	std::string id;
};

enum VertexAttribLocation {
	VertexAttribLocation_Position,
	VertexAttribLocation_Normal,
	VertexAttribLocation_Tangent,
    VertexAttribLocation_TexCoord01,
	VertexAttribLocation_Weight,
	VertexAttribLocation_BoneID,
};

/**
* 描画用オブジェクト.
*/
class Object
{
public:
	Object() : shader(0) {}
	Object(const RotTrans& rt, const Mesh* m, const Material& mat, const Shader* s) : material(mat), mesh(m), shader(s), rotTrans(rt), scale(Vector3F(1, 1, 1)), color(255, 255, 255, 255) {
		bones.resize(mesh->jointList.size(), Matrix4x3::Unit());
	}
	void Color(Color4B c) { color = c; }
	Color4B Color() const { return color; }
	Color4B ActualColor() const { return color * material.color; }
	GLubyte Metallic() const { return material.metallic; }
	GLubyte Roughness() const { return material.roughness; }
	const RotTrans& RotTrans() const { return rotTrans; }
	void SetRoughness(GLubyte r) { material.roughness = r; }
	const Mesh* Mesh() const { return mesh; }
	const Shader* Shader() const { return shader; }
	const GLfloat* GetBoneMatirxArray() const { return bones[0].f; }
	size_t GetBoneCount() const { return bones.size(); }
	bool IsValid() const { return shader; }
	void Update(float t);
	void SetAnimation(const Animation* p) { animationPlayer.SetAnimation(p); }
	void SetCurrentTime(float t) { animationPlayer.SetCurrentTime(t); }
	float GetCurrentTime() const { return animationPlayer.GetCurrentTime(); }
	void SetRotation(const Quaternion& r) { rotTrans.rot = r; }
	void SetTranslation(const Vector3F& t) { rotTrans.trans = t; }
	void Scale(const Vector3F& s) { scale = s; }
	const Vector3F& Scale() const { return scale; }

private:
	Material material;
	const ::Mesh* mesh;
	const ::Shader* shader;

	::RotTrans rotTrans;
	Vector3F scale;
	Color4B color;
	AnimationPlayer animationPlayer;
	std::vector<Matrix4x3>  bones;
};

struct android_app;

/**
* 描画処理クラス.
*/
class Renderer
{
public:
	explicit Renderer(android_app*);
	Renderer() = delete;
	~Renderer();
	Object CreateObject(const char* meshName, const Material& m, const char* shaderName);
	const Animation* GetAnimation(const char* name);
	void Initialize();
	void Render(const Object*, const Object*);
	void Update( float dTime, const Vector3F&, const Vector3F&);
	void Unload();
	void InitMesh();
	void InitTexture();

	bool HasDisplay() const { return display; }
	bool HasSurface() const { return surface; }
	bool HasContext() const { return context; }
	int32_t Width() const { return width; }
	int32_t Height() const { return height; }
	void Swap();

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
	void LoadMesh(const char*, const char* = nullptr, const char* = nullptr);
	void CreateSkyboxMesh();
	void CreateBoardMesh(const char*, const Vector3F&);
	void CreateFloorMesh(const char*, const Vector3F&);
	void CreateAsciiMesh(const char*);
	void DrawFont(const Position2F&, const char*);

private:
	android_app* state;
	bool isInitialized;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	int32_t width;
	int32_t height;

	std::string texBaseDir;

	Vector3F cameraPos;
	Vector3F cameraDir;

	uint64_t startTime;
	uint32_t frames;
	uint32_t prevFrames;

	GLuint fboMain, fboSub, fboShadow0, fboShadow1, fboHDR[5];
	GLuint depth;

	GLuint vbo;
	GLintptr vboEnd;
	GLuint ibo;
	GLintptr iboEnd;
	std::map<std::string, Shader> shaderList;
	std::map<std::string, Mesh> meshList;
	std::map<std::string, Animation> animationList;
	std::map<std::string, Texture::TexturePtr> textureList;
};

inline Position2F& Position2F::operator+=(const Vector2F& rhs) { x += rhs.x; y += rhs.y; return *this; }
inline Position2F operator+(const Position2F& lhs, const Vector2F& rhs) { return Position2F(lhs) += rhs; }
inline Position2F& Position2F::operator-=(const Vector2F& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
inline Position2F operator-(const Position2F& lhs, const Vector2F& rhs) { return Position2F(lhs) -= rhs; }
inline Vector2F operator-(const Position2F& lhs, const Position2F& rhs) { return Vector2F(lhs.x - rhs.x, lhs.y - rhs.y); }

inline Position3F& Position3F::operator+=(const Vector3F& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
inline Position3F Position3F::operator+(const Vector3F& rhs) const { return Position3F(*this) += rhs; }
inline Position3F& Position3F::operator-=(const Vector3F& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
inline Position3F Position3F::operator-(const Vector3F& rhs) const { return Position3F(*this) -= rhs; }
inline Position3F& Position3F::operator*=(const Vector3F& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
inline Position3F Position3F::operator*(const Vector3F& rhs) const { return Position3F(*this) *= rhs; }
inline Vector3F operator-(const Position3F& lhs, const Position3F& rhs) { return Vector3F(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }

inline Vector4F& Vector4F::operator*=(const Matrix4x4& rhs) {
	const Vector4F tmp(*this);
	x = tmp.Dot(rhs.GetVector(0));
	y = tmp.Dot(rhs.GetVector(1));
	z = tmp.Dot(rhs.GetVector(2));
	w = tmp.Dot(rhs.GetVector(2));
	return *this;
}
inline Vector4F operator*(const Vector4F& lhs, const Matrix4x4& rhs) { return Vector4F(lhs) *= rhs; }
inline Vector4F operator*(const Matrix4x4& lhs, const Vector4F& rhs) {
	return Vector4F(
		lhs.GetRawVector(0).Dot(rhs),
		lhs.GetRawVector(1).Dot(rhs),
		lhs.GetRawVector(2).Dot(rhs),
		lhs.GetRawVector(3).Dot(rhs)
	);
}

inline void Matrix4x4::Decompose(Quaternion* q, Vector3F* scale, Vector3F* trans) const {
	if (trans) {
		*trans = GetRawVector(3).ToVec3();
	}
	Vector3F v0 = GetVector(0).ToVec3();
	Vector3F v1 = GetVector(1).ToVec3();
	Vector3F v2 = GetVector(2).ToVec3();
	const Vector3F s(v0.Length(), v1.Length(), v2.Length());
	if (scale) {
		*scale = s;
	}
	if (q) {
		// ここ(http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/)のAlternative Methodを使う.
		v0 /= s.x;
		v1 /= s.y;
		v2 /= s.z;
		*q = Quaternion(
			sqrt(std::max(0.0f, 1.0f + v0.x - v1.y - v2.z)),
			sqrt(std::max(0.0f, 1.0f - v0.x + v1.y - v2.z)),
			sqrt(std::max(0.0f, 1.0f - v0.x - v1.y + v2.z)),
			sqrt(std::max(0.0f, 1.0f + v0.x + v1.y + v2.z)));
		*q *= 0.5f;
		q->x = copysignf(q->x, v2.y - v1.z);
		q->y = copysignf(q->y, v0.z - v2.x);
		q->z = copysignf(q->z, v1.x - v0.y);
	}
}

inline uint8_t FloatToFix8(float f) { return static_cast<uint8_t>(std::max<float>(0, std::min<float>(255, f * 255.0f + 0.5f))); }
inline float Fix8ToFloat(uint8_t n) { return static_cast<float>(n) * (1.0f / 255.0f); }

#endif // MT_RENDERER_H_INCLUDED