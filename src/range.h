#pragma once

#include "event.h"

#include "global_defines.h"
#include <glm/vec3.hpp>

namespace g3d {
struct RangeChangedEvent {
	glm::vec3 newHigh;
	glm::vec3 newLow;
};

class Range {
private:
	EventRouter* _eventRouter;

	// The x, y coordinates of graph model vertices always range from
	// -1.0f to 1.0f. The range determines how those coordinates map to
	// function input space with a simple relation. See toModelSpace().

	// Ranges are stored in high/low vectors for convenience in
	// model/function space transform.
	// high = { Xhigh, Yhigh, Zhigh }
	// low = { Xlow, Ylow, Zlow }
	glm::vec3 _high;
	glm::vec3 _low;

public:
	Range(EventRouter& eventRouter, float range)
	: _eventRouter { &eventRouter },
	  _high { range, range, range },
	  _low { -range, -range, -range }
	{}

	glm::vec3 toModelSpace(const glm::vec3& funcSpace) const;

	const glm::vec3& high() const { return _high; }
	const glm::vec3& low() const { return _low; }

	void high(const glm::vec3& h);
	void low(const glm::vec3& l);

	// Gets the model space coordinates of the origin of the graph.
	glm::vec3 origin() const;
};
}
