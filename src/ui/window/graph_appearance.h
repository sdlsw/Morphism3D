#pragma once

#include "figure/graph.h"
#include "ui/common_render.h"

namespace g3d {
struct GraphColorPreset {
	std::string title;
	bool colorMode;
	glm::vec3 nxnyColor;
	glm::vec3 pxnyColor;
	glm::vec3 nxpyColor;
	glm::vec3 pxpyColor;
};

class GraphAppearanceWindow : public UiWindow {
private:
	std::string _title;
	
	GraphAppearance* _appearance;
	bool _colorMode { true };

	void loadColorPreset(const GraphColorPreset& preset);
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	GraphAppearanceWindow(
		unsigned int id,
		GraphAppearance& appearance
	)
	: _title { std::format("Graph (figID {}) Appearance", id) },
	  _appearance { &appearance } {}
};
};
