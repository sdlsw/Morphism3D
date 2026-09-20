#pragma once

#include "figure/graph.h"
#include "ui/common_render.h"

namespace g3d {
class GraphAppearanceWindow : public UiWindow {
private:
	std::string _title;
	
	GraphAppearance* _appearance;
	bool _colorMode { true };
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	GraphAppearanceWindow(
		unsigned int id,
		GraphAppearance& appearance
	)
	: _title { std::format("Graph Appearance##{}", id) },
	  _appearance { &appearance } {}
};
};
