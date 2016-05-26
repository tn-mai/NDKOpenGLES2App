#ifndef MAI_COMMON_QUATERNION_H_INCLUDED
#define MAI_COMMON_QUATERNION_H_INCLUDED
#include "Vector.h"

namespace Mai {

struct Matrix4x3;

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
  Quaternion operator-() const { return Quaternion(-x, -y, -z, -w); }
  Quaternion& operator*=(GLfloat rhs) { x *= rhs; y *= rhs; z *= rhs; w *= rhs;  return *this; }
  Quaternion operator*(GLfloat rhs) const { return Quaternion(*this) *= rhs; }
  friend Quaternion operator*(GLfloat lhs, const Quaternion& rhs) { return rhs * lhs; }
  Quaternion& operator/=(GLfloat rhs) { x /= rhs; y /= rhs; z /= rhs; w /= rhs;  return *this; }
  constexpr Quaternion operator/(GLfloat rhs) const { return Quaternion(x / rhs, y / rhs, z / rhs, w / rhs); }
  Quaternion& operator+=(const Quaternion& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w;  return *this; }
  Quaternion operator+(const Quaternion& rhs) const { return Quaternion(*this) += rhs; }
  Quaternion& operator*=(const Quaternion& rhs) { *this = *this * rhs; return *this; }
  Quaternion operator*(const Quaternion& rhs) const {
    return Quaternion(
      w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
      w * rhs.y + y * rhs.w + z * rhs.x - x * rhs.z,
      w * rhs.z + z * rhs.w + x * rhs.y - y * rhs.x,
      w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
  }
  constexpr GLfloat Dot(const Quaternion& q) const { return x * q.x + y * q.y + z * q.z + w * q.w; }
  GLfloat Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
  Quaternion& Normalize() {
    const float l = Length();
    if (l > FLT_EPSILON) {
       return operator/=(l);
    }
    return *this;
  }
  constexpr Quaternion Conjugate() const { return Quaternion(-x, -y, -z, w); }
  Quaternion Inverse() const { return Conjugate() / Length(); }
  Vector3F Apply(const Vector3F& v) const {
    const Vector3F xyz(x, y, z);
    Vector3F uv = xyz.Cross(v);
    Vector3F uuv = xyz.Cross(uv);
    uv *= 2.0f * w;
    uuv *= 2.0f;
    return v + uv + uuv;
  }
  // @ref http://nic-gamedev.blogspot.jp/2011/11/quaternion-math-getting-local-axis.html
  constexpr Vector3F ForwardVector() const {
    return Vector3F(
      -2 * (x * z + w * y),
      2 * (w * x - y * z),
      -(1 - 2 * (x * x + y * y))
    );
  }
  constexpr Vector3F UpVector() const {
    return Vector3F(
      2 * (x * y - w * z),
      1 - 2 * (x * x + z * z),
      2 * (y * z + w * x)
    );
  }
  constexpr Vector3F RightVector() const {
    return Vector3F(
      1 - 2 * (y * y + z * z),
      2 * (x * y + w * z),
      2 * (x * z - w * y)
    );
  }
  struct AxisAngle {
    Vector3F axis;
    GLfloat angle;
  };
  AxisAngle GetAxisAngle() const {
    const float angle = 2.0f * std::acos(w);
    const float n = 1.0f / std::sqrt(1.0f - w * w);
    const Vector3F axis(x * n, y * n, z * n);
    return { axis, angle };
  }
  friend Quaternion Sleap(const Quaternion& qa, const Quaternion& qb, float t);
  static constexpr Quaternion Unit() { return Quaternion(0, 0, 0, 1); }
};
Quaternion Sleap(const Quaternion& qa, const Quaternion& qb, float t);
Matrix4x3 ToMatrix(const Quaternion& q);

} // namespace Mai

#endif // MAI_COMMON_QUATERNION_H_INCLUDED
