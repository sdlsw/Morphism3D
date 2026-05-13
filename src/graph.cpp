#include "graph.h"

namespace g3d {
glm::vec3 calcTriangleNormal(
	const Position& thisPosition,
	const Position& other1,
	const Position& other2
) {
	return glm::normalize(glm::cross(
		other1.vec-thisPosition.vec, other2.vec-thisPosition.vec
	));
}
}
