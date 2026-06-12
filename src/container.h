#pragma once

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

struct RenderSettings {
	bool renderAxes = true;
	bool renderFrame = true;
	bool renderLightObject = false;
};

struct DebugSettings {
	bool renderTestObject = false;
};
}
