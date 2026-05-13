#include "primitive.h"

namespace g3d {
g3d::StaticMesh cubeMesh(g3d::Renderer& renderer, float size) {
	float halfSize = size / 2.0f;
	std::vector<g3d::Position> positions {
		{ -halfSize, -halfSize, halfSize },
		{ halfSize, -halfSize, halfSize },
		{ -halfSize, -halfSize, -halfSize },
		{ halfSize, -halfSize, -halfSize },
		{ -halfSize, halfSize, halfSize },
		{ halfSize, halfSize, halfSize },
		{ -halfSize, halfSize, -halfSize },
		{ halfSize, halfSize, -halfSize },
	};
	std::vector<uint16_t> indices {
		0, 2, 1,
		1, 2, 3,
		1, 3, 5,
		5, 3, 7,
		4, 5, 6,
		5, 7, 6,
		4, 6, 0,
		0, 6, 2,
		4, 0, 1,
		4, 1, 5,
		6, 7, 3,
		6, 3, 2
	};

	return { renderer, positions, indices };
}

g3d::StaticVertexAttributes<g3d::Color> solidColor(
	const g3d::StaticMesh& mesh,
	const g3d::Color& color
) {
	std::vector<g3d::Color> colors { mesh.positionCount(), color };
	return { mesh.renderer(), colors };
}
}
