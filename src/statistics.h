#pragma once

#include "temporal.h"

#include <concepts>
#include <deque>
#include <unordered_map>

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

	T getAverage() {
		T sum { static_cast<T>(0) };

		// TODO: For now, use simple method of adding every
		// element in _contents to average every time. Not the most
		// efficient but more precise.
		for (const auto& x : _contents) {
			sum += x;
		}

		return sum / static_cast<T>(_contents.size());
	}

	size_t size() {
		return _contents.size();
	}

	T getMeasurement(size_t i) {
		return _contents[i];
	}
};

class Timer {
private:
	bool _inMeasurement = false;
	TimePoint _start;
	MovingAverage<float> _measurements;

public:
	Timer(size_t maxSize) : _measurements { maxSize } {}

	void start();
	void stop();

	MovingAverage<float>& measurements() { return _measurements; }
};

class TimerCollection {
private:
	size_t _defaultTimerSize;
	std::unordered_map<std::string, Timer> _timers;

public:
	TimerCollection(size_t defaultTimerSize) : _defaultTimerSize { defaultTimerSize } {}

	void start(const std::string& key);
	void stop(const std::string& key);
	float getAverage(const std::string& key);
	Timer& getTimer(const std::string& key);
};
}
