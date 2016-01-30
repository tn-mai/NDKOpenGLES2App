#include "Matrix.h"
#include "Quaternion.h"
#include <algorithm>
#include <utility>

namespace Mai {

Matrix4x4& Matrix4x4::Scale(float x, float y, float z) {
	SetVector(0, GetVector(0) * x);
	SetVector(1, GetVector(1) * y);
	SetVector(2, GetVector(2) * z);
	return *this;
}

Matrix4x4& Matrix4x4::Transpose() {
	std::swap(f[1], f[4]);
	std::swap(f[2], f[8]);
	std::swap(f[3], f[12]);
	std::swap(f[6], f[9]);
	std::swap(f[7], f[13]);
	std::swap(f[11], f[14]);
	return *this;
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

void Matrix4x4::Decompose(Quaternion* q, Vector3F* scale, Vector3F* trans) const {
  if (trans) {
    *trans = GetVector(3).ToVec3();
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

Matrix4x4 Matrix4x4::Translation(float x, float y, float z) {
	Matrix4x4 m = Unit();
	m.Set(3, 0, x);
	m.Set(3, 1, y);
	m.Set(3, 2, z);
	return m;
}

Matrix4x4 Matrix4x4::FromScale(float x, float y, float z) {
  Matrix4x4 m;
  m.SetVector(0, Vector4F(x, 0, 0, 0));
  m.SetVector(1, Vector4F(0, y, 0, 0));
  m.SetVector(2, Vector4F(0, 0, z, 0));
  m.SetVector(3, Vector4F(0, 0, 0, 1));
  return m;
}

Matrix4x4 Matrix4x4::RotationX(float angle/* radian */) {
  const float c = cos(angle);
  const float s = sin(angle);
  Matrix4x4 m;
  m.SetVector(0, Vector4F(1, 0, 0, 0));
  m.SetVector(1, Vector4F(0, c, -s, 0));
  m.SetVector(2, Vector4F(0, s, c, 0));
  m.SetVector(3, Vector4F(0, 0, 0, 1));
  return m;
}

Matrix4x4 Matrix4x4::RotationY(float angle/* radian */) {
  const float c = cos(angle);
  const float s = sin(angle);
  Matrix4x4 m;
  m.SetVector(0, Vector4F(c, 0, s, 0));
  m.SetVector(1, Vector4F(0, 1, 0, 0));
  m.SetVector(2, Vector4F(-s, 0, c, 0));
  m.SetVector(3, Vector4F(0, 0, 0, 1));
  return m;
}

Matrix4x4 Matrix4x4::RotationZ(float angle/* radian */) {
  const float c = cos(angle);
  const float s = sin(angle);
  Matrix4x4 m;
  m.SetVector(0, Vector4F(c, -s, 0, 0));
  m.SetVector(1, Vector4F(s, c, 0, 0));
  m.SetVector(2, Vector4F(0, 0, 1, 0));
  m.SetVector(3, Vector4F(0, 0, 0, 1));
  return m;
}

Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& rhs) {
	const Matrix4x4 m = Matrix4x4(*this).Transpose();
	for (int i = 0; i < 4; ++i) {
		const Vector4F v = rhs.GetVector(i);
		SetVector(i, Vector4F(m.GetVector(0).Dot(v), m.GetVector(1).Dot(v), m.GetVector(2).Dot(v), m.GetVector(3).Dot(v)));
	}
	return *this;
}

Matrix4x3& Matrix4x3::operator*=(const Matrix4x3& rhs) {
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

Vector4F& operator*=(Vector4F&& lhs, const Matrix4x4& rhs) {
  const Vector4F tmp(lhs);
  lhs.x = tmp.Dot(rhs.GetVector(0));
  lhs.y = tmp.Dot(rhs.GetVector(1));
  lhs.z = tmp.Dot(rhs.GetVector(2));
  lhs.w = tmp.Dot(rhs.GetVector(2));
  return lhs;
}

Vector4F operator*(const Matrix4x4& lhs, const Vector4F& rhs) {
  return Vector4F(
    lhs.GetRawVector(0).Dot(rhs),
    lhs.GetRawVector(1).Dot(rhs),
    lhs.GetRawVector(2).Dot(rhs),
    lhs.GetRawVector(3).Dot(rhs)
  );
}

} // namespace Mai
