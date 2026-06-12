#include "temporal.h"

namespace g3d {
TimePoint now() {
	return std::chrono::high_resolution_clock::now();
}

float secondsSince(const TimePoint& before) {
	auto after = now();
	return std::chrono::duration<float, std::chrono::seconds::period>(after - before).count();
}
}
