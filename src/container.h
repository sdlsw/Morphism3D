#pragma once

#include <format>
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
	DiscriminatedStringMap(const std::string& suffix) : _suffix { suffix } {}
	const std::string& operator[](const std::string& key);
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
