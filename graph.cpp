#include "graph.h"

namespace g3d {
std::vector<g3d::Vertex> recolor(
	const std::vector<g3d::Vertex>& verts,
	const glm::vec3& newColor
) {
	std::vector<g3d::Vertex> out;

	for (const auto& v : verts) {
		out.push_back({v.pos, newColor});
	}

	return out;
}

glm::vec3 calcTriangleNormal(
	const Vertex& thisVertex,
	const Vertex& other1,
	const Vertex& other2
) {
	return glm::normalize(glm::cross(
		other1.pos-thisVertex.pos, other2.pos-thisVertex.pos
	));
}
}
