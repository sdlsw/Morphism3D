#include "statistics.h"

#include <stdexcept>

namespace g3d {
void TimeMeasurement::begin() {
	_inMeasurement = true;
	_start = now();
}

void TimeMeasurement::end() {
	if (!_inMeasurement) {
		throw std::runtime_error("TimeMeasurement::end(): Not in measurement");
	}

	_inMeasurement = false;
	_measurement.put(secondsSince(_start));
}

float TimeMeasurement::get() {
	return _measurement.get();
}
}
