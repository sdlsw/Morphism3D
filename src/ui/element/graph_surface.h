#pragma once

#include "expression.h"
#include "figure/graph_surface.h"
#include "figure/figure.h"
#include "ui/common_graph.h"

namespace g3d {
class SurfaceGraphAppearanceWindow : public UiWindow {
private:
	std::string _title;
	GraphColorPanel<2> _colorPanel;
	WithInitial<Material>* _material;

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	SurfaceGraphAppearanceWindow(
		unsigned int id,
		GraphColors<2>& colors,
		WithInitial<Material>& material,
		std::span<const GraphColorPreset<2>> presets
	)
	: _title { std::format("Graph (figID {}) Appearance", id) },
	  _colorPanel { colors, presets },
	  _material { &material }
	{}
};

class SurfaceGraphElement : public UiElement {
private:
	const std::string _title;
	std::array<char, 255> _expressionBuf;

	bool _clampOut;

	SurfaceGraphFigure* _graph;
	int _cells;

	void expressionInput();
	void gridToggle();
	void renderModeSlider();
	void resolutionInput();
	void renderSettings();

	DiscriminatedStringMap _guiIds;

	SurfaceGraphAppearanceWindow _appearanceWindow;
public:
	const std::string& title() const override { return _title; }
	unsigned int id() const override { return _graph->id(); }
	void show() override;

	SurfaceGraphElement() = delete;
	SurfaceGraphElement(SurfaceGraphFigure& graph);
};

template<>
struct UiElementForFigure<SurfaceGraphFigure> {
	typedef SurfaceGraphElement value;
};
}
