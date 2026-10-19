#pragma once

#include "global_defines.h"
#include <glm/glm.hpp>

#include <format>
#include <functional>
#include <unordered_map>

namespace g3d {
template<typename T>
struct WithInitial {
private:
	T _initial;

public:
	T current;

	WithInitial(const T& init) : _initial { init }, current { init } {}
	operator T&() { return current; }
	operator T const &() const { return current; }
	operator T() const { return current; }
	void reset() { current = _initial; }

	const T& initial() const { return _initial; }
};

// Simple helper container for things like ImGui widget IDs and timer names.
// Adds suffixes to strings and caches the results. Helps avoid spamming
// std::string members on classes that need lots of string names that differ
// only by some ID number/suffix.
struct DiscriminatedStringMap {
private:
	std::string _suffix;
	std::unordered_map<std::string, std::string> _cache;

public:
	DiscriminatedStringMap(const std::string& suffix)
	: _suffix { suffix } {}

	const std::string& operator[](const std::string& key);

	// shortcut function for `map[key].c_str()`, common for ImGui usage
	const char* c_str(const std::string& key);

	const auto& cache() const { return _cache; }
};

// Recursively defined structure implementing a fixed size binary tree where
// every leaf is filled.
template<typename T, size_t DEPTH>
struct FixedSizeBinaryTree {
	static constexpr size_t spanSize = 1 << DEPTH;

	FixedSizeBinaryTree<T, DEPTH-1> left;
	FixedSizeBinaryTree<T, DEPTH-1> right;

	void copyFromSpan(const std::span<T, spanSize>& span) {
		constexpr size_t halfSize = 1 << (DEPTH-1);
		std::span<T, halfSize> leftSpan { span.data(), halfSize };
		left.copyFromSpan(leftSpan);
		std::span<T, halfSize> rightSpan { span.data() + halfSize, halfSize };
		right.copyFromSpan(rightSpan);
	}

	static FixedSizeBinaryTree<T, DEPTH> fromSpan(const std::span<T, spanSize>& span) {
		FixedSizeBinaryTree<T, DEPTH> tmp;
		tmp.copyFromSpan(span);
		return tmp;
	}

	std::span<T, spanSize> getSpan() {
		std::span<T, spanSize> s { reinterpret_cast<T*>(this), spanSize };
		return s;
	}
};

template<typename T>
struct FixedSizeBinaryTree<T, 0> {
	T value;

	void copyFromSpan(const std::span<T, 1>& span) {
		value = span[0];
	}

	static FixedSizeBinaryTree<T, 0> fromSpan(const std::span<T, 1>& span) {
		return { span[0] };
	}

	std::span<T, 1> getSpan() {
		std::span<T, 1> s { &value, 1 };
		return s;
	}
};

struct RenderSettings {
	bool renderAxes = true;
	bool renderFrame = true;
	bool renderLightObject = false;
};

struct DebugSettings {
	bool renderTestObject = false;
};
}
