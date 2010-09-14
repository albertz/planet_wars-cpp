/*
 *  FixedPointNumber.h
 *  OpenLieroX
 *
 *  Created by Albert Zeyer on 27.12.09.
 *  code under LGPL
 *
 */

#ifndef __OLX_FIXEDPOINTNUMBER_H___
#define __OLX_FIXEDPOINTNUMBER_H___

#include <SDL.h> // Uint64

template< Uint64 _factor >
struct FixedPointNumber {
	static const Uint64 factor = _factor;
	Sint64 number;
	
	FixedPointNumber() : number(0) {}
	FixedPointNumber(int n) : number(n * factor) {}
	FixedPointNumber(Sint64 n) : number(n * factor) {}
	FixedPointNumber(double n) : number(Sint64(n * factor)) {}
	static FixedPointNumber raw(Sint64 num) { FixedPointNumber n; n.number = num; return n; }
	static FixedPointNumber minGreater0() { return raw(1); }
	
	FixedPointNumber& operator++/* prefix */(int) { number += factor; return *this; }
	FixedPointNumber& operator--/* prefix */(int) { number -= factor; return *this; }
	FixedPointNumber& operator+=(const FixedPointNumber& n) { number += n.number; return *this; }
	FixedPointNumber& operator-=(const FixedPointNumber& n) { number -= n.number; return *this; }

	FixedPointNumber operator-() const { return raw(-number); }	
	FixedPointNumber operator+(const FixedPointNumber& n) const { return raw(number + n.number); }
	FixedPointNumber operator-(const FixedPointNumber& n) const { return raw(number - n.number); }
	FixedPointNumber operator*(const FixedPointNumber& n) const { return raw(number * n.number / Sint64(factor)); }
	double operator*(double n) const { return number * (n / factor); }
	FixedPointNumber operator/(const FixedPointNumber& n) const { return raw(number * factor / n.number); }
	FixedPointNumber operator%(const FixedPointNumber& n) const { return raw(number % n.number); }
	
	bool operator==(const FixedPointNumber& n) const { return number == n.number; }
	bool operator!=(const FixedPointNumber& n) const { return !(*this == n); }
	bool operator<(const FixedPointNumber& n) const { return number < n.number; }
	bool operator>(const FixedPointNumber& n) const { return number > n.number; }
	bool operator<=(const FixedPointNumber& n) const { return number <= n.number; }
	bool operator>=(const FixedPointNumber& n) const { return number >= n.number; }
	
	int asInt() const { return (int)(number / Sint64(factor)); }
	double asDouble() const { return double(number) / factor; }
	FixedPointNumber abs() const { return raw((number >= 0) ? number : -number); }
	FixedPointNumber floor() const { return raw((number >= 0) ? (number - number % factor) : (number - factor + 1 + (-number - 1) % factor)); }
};

#endif
