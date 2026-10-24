#pragma once

#include "expression.h"
#include "figure/graph.h"
#include "figure/figure.h"
#include "ui/common.h"
#include "ui/window/graph_appearance.h"

namespace g3d {
// Unfortunately need to define this in header since it's used in a template.
constexpr const char SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP[] {
	"This setting cannot be changed outside Surface rendering mode."
};

class SurfaceGraphElement : public UiElement {
private:
	const std::string _title;
	std::array<char, 255> _expressionBuf;

	bool _clampZ;

	SurfaceGraphFigure* _graph;
	int _cells;

	void expressionInput();
	void gridToggle();
	void renderModeSlider();
	void resolutionInput();
	void renderSettings();

	DiscriminatedStringMap _guiIds;

	GraphAppearanceWindow<2> _appearanceWindow;
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
