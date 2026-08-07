#include "ui/graph_window.h"

static const std::string RENDER_MODE_SURFACE_NAME = "Surface";
static const std::string RENDER_MODE_WIREFRAME_NAME = "Wireframe";
static const std::string RENDER_MODE_NONE_NAME = "None";
static const std::string RENDER_MODE_UNKNOWN_NAME { g3d::UI_UNKNOWN_MODE_NAME };

namespace g3d {
const std::string& renderModeName(const GraphRenderMode& mode) {
	switch (mode) {
		case GraphRenderMode::surface:
			return RENDER_MODE_SURFACE_NAME;
		case GraphRenderMode::wireframe:
			return RENDER_MODE_WIREFRAME_NAME;
		case GraphRenderMode::none:
			return RENDER_MODE_NONE_NAME;
		default:
			return RENDER_MODE_UNKNOWN_NAME;
	}
}
}
