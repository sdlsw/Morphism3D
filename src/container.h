#pragma once

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

// Caches the results of a function of the form
//
//   std::string f(const Arg&) { ... }
template<typename Arg>
class CachedStringFunction {
private:
	std::function<std::string(const Arg&)> _func;
	std::unordered_map<Arg, std::string> _cache;

public:
	CachedStringFunction(const std::function<std::string(Arg)>& func)
	: _func { func } {}

	const std::string& operator[](const Arg& key) {
		if (!_cache.contains(key)) {
			_cache.emplace(key, _func(key));
		}

		return _cache.at(key);
	}

	const char* c_str(const Arg& key) {
		return (*this)[key].c_str();
	}

	const auto& cache() const { return _cache; }
};

// Simple helper container for things like ImGui widget IDs and timer names.
// Adds suffixes to strings and caches the results. Helps avoid spamming
// std::string members on classes that need lots of string names that differ
// only by some ID number/suffix.
struct DiscriminatedStringMap : public CachedStringFunction<std::string> {
public:
	DiscriminatedStringMap(const std::string& suffix)
	: CachedStringFunction<std::string>([suffix](const std::string& key) {
		return std::format("{}{}", key, suffix);
	}) {}
};

// Recursively defined structure implementing a fixed size binary tree where
// every leaf is filled.
template<typename T, size_t DEPTH>
struct FixedSizeBinaryTree {
	static constexpr size_t spanSize = 1 << DEPTH;

	FixedSizeBinaryTree<T, DEPTH-1> left;
	FixedSizeBinaryTree<T, DEPTH-1> right;

	void copyFromSpan(std::span<const T, spanSize> span) {
		constexpr size_t halfSize = 1 << (DEPTH-1);
		std::span<const T, halfSize> leftSpan { span.data(), halfSize };
		left.copyFromSpan(leftSpan);
		std::span<const T, halfSize> rightSpan { span.data() + halfSize, halfSize };
		right.copyFromSpan(rightSpan);
	}

	void fill(const T& val) {
		for (auto& x : getSpan()) {
			x = val;
		}
	}

	static FixedSizeBinaryTree<T, DEPTH> fromSpan(std::span<const T, spanSize> span) {
		FixedSizeBinaryTree<T, DEPTH> tmp;
		tmp.copyFromSpan(span);
		return tmp;
	}

	static FixedSizeBinaryTree<T, DEPTH> fromValue(const T& val) {
		FixedSizeBinaryTree<T, DEPTH> tmp;
		tmp.fill(val);
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

	void copyFromSpan(std::span<const T, 1> span) {
		value = span[0];
	}

	void fill(const T& val) {
		value = val;
	}

	static FixedSizeBinaryTree<T, 0> fromSpan(std::span<const T, 1> span) {
		return { span[0] };
	}

	static FixedSizeBinaryTree<T, 0> fromValue(const T& val) {
		return { val };
	}

	std::span<T, 1> getSpan() {
		std::span<T, 1> s { &value, 1 };
		return s;
	}
};
}
