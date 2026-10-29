#pragma once

// Application settings structures shared by multiple files.

namespace g3d {
struct RenderSettings {
	bool renderAxes = true;
	bool renderFrame = true;
	bool renderLightObject = false;
};

struct DebugSettings {
	bool renderTestObject = false;
	bool showGraphDebug = false;
};
}
