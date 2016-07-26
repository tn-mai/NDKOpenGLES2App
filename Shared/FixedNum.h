/**
* @file FixedNum.h
*/
#include <stdint.h>

namespace Mai {

  /**
  * A fixed point number.
  */
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

} // namespace Mai
