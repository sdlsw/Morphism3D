#pragma once

#include "vk/renderer.h"

namespace g3d {
g3d::StaticMesh cubeMesh(g3d::Renderer& renderer, float size);

// Creates a solid color resource that matches an existing mesh in size.
g3d::StaticVertexAttributes<g3d::Color> solidColor(
	const g3d::StaticMesh& mesh,
	const g3d::Color& color
);
}
