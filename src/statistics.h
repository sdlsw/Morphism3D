#pragma once

#include "temporal.h"

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

class TimeMeasurement {
private:
	bool _inMeasurement = false;
	TimePoint _start;
	MovingAverage<float> _measurement;

public:
	TimeMeasurement(size_t maxSize) : _measurement { maxSize } {}

	void begin();
	void end();
	float get();
};
}
