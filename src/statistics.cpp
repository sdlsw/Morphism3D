#include "statistics.h"

#include <stdexcept>

namespace g3d {
void Timer::start() {
	_inMeasurement = true;
	_start = now();
}

void Timer::stop() {
	if (!_inMeasurement) {
		throw std::runtime_error("Timer::stop(): Not in measurement");
	}

	_inMeasurement = false;
	_measurements.put(secondsSince(_start));
}

Timer& TimerCollection::getTimer(const std::string& key) {
	if (!hasTimer(key)) {
		_timers.insert({key, Timer(_defaultTimerSize)});
	}

	return _timers.at(key);
}

bool TimerCollection::hasTimer(const std::string& key) {
	return _timers.contains(key);
}

void TimerCollection::removeTimer(const std::string& key) {
	if (!hasTimer(key)) return;
	_timers.erase(key);
}

void TimerCollection::start(const std::string& key) {
	getTimer(key).start();
}

void TimerCollection::stop(const std::string& key) {
	getTimer(key).stop();
}

float TimerCollection::getAverage(const std::string& key) {
	return getTimer(key).measurements().getAverage();
}
}
