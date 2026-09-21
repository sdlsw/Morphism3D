#pragma once

#include <utility>

namespace g3d {
template<typename T>
struct enableBitmaskOps {
	static constexpr bool value = false;
};

template<typename T>
concept BitmaskOpsEnabled = enableBitmaskOps<T>::value;

template<BitmaskOpsEnabled T>
constexpr T operator|(const T& a, const T& b) {
	return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
}

template<BitmaskOpsEnabled T>
T& operator|=(T& a, const T& b) {
	a = a | b;
	return a;
}

template<BitmaskOpsEnabled T>
constexpr T operator&(const T& a, const T& b) {
	return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
}

template<BitmaskOpsEnabled T>
T& operator &=(T& a, const T& b) {
	a = a & b;
	return a;
}

template<BitmaskOpsEnabled T>
constexpr T operator~(const T& a) {
	return static_cast<T>(~std::to_underlying(a));
}

template<BitmaskOpsEnabled T>
constexpr bool any(const T& a) {
	return std::to_underlying(a) != 0;
}

template<BitmaskOpsEnabled T>
constexpr T allEnabled() {
	return static_cast<T>(~static_cast<std::underlying_type_t<T>>(0));
}

// Tests if a is a subset of b
template<BitmaskOpsEnabled T>
constexpr bool isSubset(const T& a, const T& b) {
	const auto& aUnder = std::to_underlying(a);
	const auto& bUnder = std::to_underlying(b);

	return aUnder | bUnder == bUnder;
}

template<BitmaskOpsEnabled T>
constexpr bool flagsEnabled(const T& flags, const T& check) {
	return any(flags & check);
}
};
