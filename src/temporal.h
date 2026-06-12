#pragma once

#include <chrono>

namespace g3d {
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

TimePoint now();
float secondsSince(const TimePoint& before);
}
