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

class GraphElement : public UiElement {
private:
	const std::string _title;
	std::array<char, 255> _expressionBuf;

	bool _clampZ;

	GraphFigure* _graph;
	int _cells;

	void expressionInput();
	void gridToggle();
	void renderModeSlider();
	void resolutionInput();
	void renderSettings();

	DiscriminatedStringMap _guiIds;

	GraphAppearanceWindow _appearanceWindow;
public:
	const std::string& title() const override { return _title; }
	unsigned int id() const override { return _graph->id(); }
	void show() override;

	GraphElement() = delete;
	GraphElement(GraphFigure& graph)
	: _graph { &graph },
	  _title { std::format("Graph (figID {})", graph.id()) },
	  _cells { static_cast<int>(graph.cells()) },
	  _clampZ { graph.clampZ() },
	  _guiIds { std::format("##{}", graph.id()) },
	  _appearanceWindow { graph.id(), graph.appearance() }
	{
		std::fill(_expressionBuf.begin(), _expressionBuf.end(), '\0');
		_appearanceWindow.open = false;
	}
};

template<>
struct UiElementForFigure<GraphFigure> {
	typedef GraphElement value;
};
}
