#include "range.h"

namespace g3d {
glm::vec3 Range::toModelSpace(const glm::vec3& funcSpace) const {
	// Simplified from (funcSpace - (H+L)/2) / ((H - L)/2), that
	// is, a translation followed by a scale.
	return (2.0f*funcSpace - _high - _low) / (_high - _low);
}

void Range::high(const glm::vec3& h) {
	RangeChangedEvent e { h, _low };
	_eventRouter->routeEvent(e);
	_high = h;
}

void Range::low(const glm::vec3& l) {
	RangeChangedEvent e { _high, l };
	_eventRouter->routeEvent(e);
	_low = l;
}

glm::vec3 Range::origin() const {
	return toModelSpace({0, 0, 0});
}
}
