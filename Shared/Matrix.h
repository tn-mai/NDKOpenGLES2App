#ifndef MAI_COMMON_MATRIX_H_INCLUDED
#define MAI_COMMON_MATRIX_H_INCLUDED
#include "Vector.h"

namespace Mai {

struct Quaternion;

/** 4x4行列.
*/
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
	Matrix4x4& Scale(float x, float y, float z);
	Matrix4x4& Transpose();
	Matrix4x4& Inverse();
	static Matrix4x4 Translation(float x, float y, float z);
	static Matrix4x4 FromScale(float x, float y, float z);
	static Matrix4x4 RotationX(float angle/* radian */);
	static Matrix4x4 RotationY(float angle/* radian */);
	static Matrix4x4 RotationZ(float angle/* radian */);
	Matrix4x4& operator*=(const Matrix4x4& rhs);
	Matrix4x4 operator*(const Matrix4x4& rhs) const { return Matrix4x4(*this) *= rhs; }
	void Decompose(Quaternion* q, Vector3F* scale, Vector3F* trans) const;
	static const Matrix4x4& Unit() {
		static const Matrix4x4 m = { { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 } };
		return m;
	}
};

/** 3x3行列 + 平行移動.
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
	Matrix4x3& operator*=(const Matrix4x3& rhs);
	Matrix4x3 operator*(const Matrix4x3& rhs) const { return Matrix4x3(*this) *= rhs; }
	static const Matrix4x3& Unit() {
		static const Matrix4x3 m = { { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0 } };
		return m;
	}
};

Vector4F& operator*=(Vector4F&& lhs, const Matrix4x4& rhs);
inline Vector4F operator*(const Vector4F& lhs, const Matrix4x4& rhs) { return Vector4F(lhs) *= rhs; }
Vector4F operator*(const Matrix4x4& lhs, const Vector4F& rhs);

} // namespace Mai

#endif // MAI_COMMON_MATRIX_H_INCLUDED
