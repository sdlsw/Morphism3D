#pragma once

#include "ui/common.h"
#include "expression.h"
#include <graph.h>

namespace g3d {
// Unfortunately need to define this in header since it's used in a template.
constexpr const char SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP[] {
	"This setting cannot be changed outside Surface rendering mode."
};

class GraphWindow : public UiWindow {
private:
	const std::string _title { "Graph" };
	std::array<char, 255> _expressionBuf;

	glm::vec3 _rangeLow;
	glm::vec3 _rangeHigh;
	bool _clampZ;

	Graph* _graph;
	int _cells;

	void expressionInput();
	void gridToggle();
	void renderModeSlider();
	void resolutionInput();
	void renderSettings();
	void rangeInput(char dim, float& low, float& high);
	void rangeInputs();

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	GraphWindow() = delete;
	GraphWindow(Graph& graph, Window& window)
	: _graph { &graph },
	  _cells { static_cast<int>(graph.cells()) },
	  _rangeLow { graph.rangeLow() },
	  _rangeHigh { graph.rangeHigh() },
	  _clampZ { graph.clampZ() }
	{
		std::fill(_expressionBuf.begin(), _expressionBuf.end(), '\0');
	}
};
}
