#pragma once
#include <concepts>
#include <deque>

namespace g3d {
template<std::floating_point T>
class MovingAverage {
private:
	std::deque<T> _contents;
	size_t _maxSize;
public:
	MovingAverage(size_t maxSize) : _maxSize {maxSize} {}

	void put(T x) {
		if (_contents.size() == _maxSize) {
			_contents.pop_back();
		}

		_contents.push_front(x);
	}

	T get() {
		T sum { static_cast<T>(0) };

		// TODO: For now, use simple method of adding every
		// element in _contents to average every time. Not the most
		// efficient but more precise.
		for (const auto& x : _contents) {
			sum += x;
		}

		return sum / static_cast<T>(_contents.size());
	}
};

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

enum class GraphRenderMode : int {
	surface = 0,
	wireframe,
	none
};

constexpr unsigned int GraphRenderModeCount = 3;

struct RenderSettings {
	bool renderAxes = true;
	bool renderGrid = true;
	bool renderNormals = false;
	bool renderLightObject = false;
	GraphRenderMode graphRenderMode = GraphRenderMode::surface;
};
}
