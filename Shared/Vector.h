#ifndef MAI_COMMON_VECTOR_H_INCLUDED
#define MAI_COMMON_VECTOR_H_INCLUDED
#include <cmath>
#include <cfloat>
#include <GLES2/gl2.h>

namespace Mai {

struct Vector2F;
struct Vector3F;
struct Vector4F;

/**
* 3D座標.
*/
struct Position3F {
	GLfloat x, y, z;
	Position3F() {}
	constexpr Position3F(GLfloat a, GLfloat b, GLfloat c) : x(a), y(b), z(c) {}
	bool operator==(const Position3F& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const Position3F& rhs) const { return !(*this == rhs); }
	Position3F operator-() const { return Position3F(-x, -y, -z); }
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
	constexpr GLfloat Dot(const Position3F& rhs) const { return x * rhs.x + y * rhs.y; }
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
	Position2F& operator*=(const Vector2F&);
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
		return Position2S(static_cast<GLushort>(a * static_cast<float>(0xffff)), static_cast<GLushort>(b * static_cast<float>(0xffff)));
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
  GLfloat Length() const { return std::sqrt(x * x + y * y); }
  Vector2F& Normalize() { return operator/=(Length()); }
  constexpr GLfloat Cross(const Vector2F& rhs) const { return x * rhs.y - y * rhs.x; }
  constexpr GLfloat Dot(const Vector2F& rhs) const { return x * rhs.x + y * rhs.y; }
  constexpr Position2F ToPos() const { return Position2F(x, y); }
  static constexpr Vector2F Unit() { return Vector2F(0, 0); }
};

struct Vector3F {
	GLfloat x, y, z;
	Vector3F() {}
	constexpr Vector3F(GLfloat a, GLfloat b, GLfloat c) : x(a), y(b), z(c) {}
	constexpr explicit Vector3F(const Position3F& p) : x(p.x), y(p.y), z(p.z) {}
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
	Vector3F& operator/=(const Vector3F& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }
	Vector3F operator/(const Vector3F& rhs) const { return Vector3F(*this) /= rhs; }
	constexpr GLfloat LengthSq() const { return Dot(*this); }
	GLfloat Length() const { return std::sqrt(LengthSq()); }
	Vector3F& Normalize() {
	  const float l = Length();
	  if (l > FLT_EPSILON) {
		return operator/=(l);
	  }
	  return *this;
	}
	constexpr Vector3F Cross(const Vector3F& rhs) const { return Vector3F(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x); }
	constexpr GLfloat Dot(const Vector3F& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
	static constexpr Vector3F Unit() { return Vector3F(0, 0, 0); }
	constexpr Position3F ToPosition3F() const { return Position3F(x, y, z); }
};
inline GLfloat Dot(const Vector3F& lhs, const Position3F& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }
inline GLfloat Dot(const Position3F& lhs, const Vector3F& rhs) { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }

struct Vector4F {
	GLfloat x, y, z, w;
	Vector4F() {}
	constexpr Vector4F(const Vector3F& v, GLfloat d = 1.0f) : x(v.x), y(v.y), z(v.z), w(d) {}
	constexpr Vector4F(GLfloat a, GLfloat b, GLfloat c, GLfloat d) : x(a), y(b), z(c), w(d) {}
	constexpr bool operator==(const Vector4F& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
	constexpr bool operator!=(const Vector4F& rhs) const { return !(*this == rhs); }
	Vector4F& operator+=(const Vector4F& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; return *this; }
	Vector4F operator+(const Vector4F& rhs) const { return Vector4F(*this) += rhs; }
	Vector4F& operator-=(const Vector4F& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w;  return *this; }
	Vector4F operator-(const Vector4F& rhs) const { return Vector4F(*this) -= rhs; }
	Vector4F& operator*=(const Vector4F& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w;  return *this; }
	Vector4F operator*(const Vector4F& rhs) const { return Vector4F(*this) *= rhs; }
	Vector4F& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; z *= rhs; w *= rhs;  return *this; }
	Vector4F operator*(GLfloat rhs) const { return Vector4F(*this) *= rhs; }
	friend Vector4F operator*(GLfloat lhs, const Vector4F& rhs) { return rhs * lhs; }
	Vector4F& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; z /= rhs; w /= rhs;  return *this; }
	Vector4F operator/(GLfloat rhs) const { return Vector4F(*this) /= rhs; }
	GLfloat Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
	Vector4F& Normalize() {
	  const float l = Length();
	  if (l > FLT_EPSILON) {
		return operator/=(l);
	  }
	  return *this;
	}
	constexpr Vector4F Cross(const Vector4F& rhs) const { return Vector4F(y * rhs.z - z * rhs.y, z * rhs.w - w * rhs.z, w * rhs.x - x * rhs.w, x * rhs.y - y * rhs.x); }
	constexpr GLfloat Dot(const Vector4F& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; }
	constexpr GLfloat Dot3(const Vector4F& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
	constexpr Vector3F ToVec3() const { return Vector3F(x, y, z); }
	static constexpr Vector4F Unit() { return Vector4F(0, 0, 0, 1); }
};

struct Color4B {
	GLubyte r, g, b, a;
	Color4B() {}
	constexpr Color4B(GLubyte rr, GLubyte gg, GLubyte bb, GLubyte aa = 255) : r(rr), g(gg), b(bb), a(aa) {}
	Color4B& operator*=(float n) { r = static_cast<GLubyte>(r * n); g = static_cast<GLubyte>(g * n); b = static_cast<GLubyte>(b * n); a = static_cast<GLubyte>(a * n); return *this; }
	Color4B& operator/=(float n) { return operator*=(1.0f / n); }
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
	Vector4F ToVector4F() const { return Vector4F(r, g, b, a) * (1.0f / 255.0f); }
};

inline Position2F& Position2F::operator+=(const Vector2F& rhs) { x += rhs.x; y += rhs.y; return *this; }
inline Position2F operator+(const Position2F& lhs, const Vector2F& rhs) { return Position2F(lhs) += rhs; }
inline Position2F& Position2F::operator-=(const Vector2F& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
inline Position2F operator-(const Position2F& lhs, const Vector2F& rhs) { return Position2F(lhs) -= rhs; }
inline Position2F& Position2F::operator*=(const Vector2F& rhs) { x *= rhs.x; y *= rhs.y; return *this; }
inline Position2F operator*(const Position2F& lhs, const Vector2F& rhs) { return Position2F(lhs) *= rhs; }
inline Vector2F operator-(const Position2F& lhs, const Position2F& rhs) { return Vector2F(lhs.x - rhs.x, lhs.y - rhs.y); }

inline Position3F& Position3F::operator+=(const Vector3F& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
inline Position3F Position3F::operator+(const Vector3F& rhs) const { return Position3F(*this) += rhs; }
inline Position3F& Position3F::operator-=(const Vector3F& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
inline Position3F Position3F::operator-(const Vector3F& rhs) const { return Position3F(*this) -= rhs; }
inline Position3F& Position3F::operator*=(const Vector3F& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }
inline Position3F Position3F::operator*(const Vector3F& rhs) const { return Position3F(*this) *= rhs; }
inline Vector3F operator-(const Position3F& lhs, const Position3F& rhs) { return Vector3F(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z); }

} // namespace Mai
#endif // MAI_COMMON_VECTOR_H_INCLUDED
