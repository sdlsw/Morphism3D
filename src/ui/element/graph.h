#pragma once

#include "expression.h"
#include "figure/graph.h"
#include "figure/figure.h"
#include "ui/common.h"

namespace g3d {
// Unfortunately need to define this in header since it's used in a template.
constexpr const char SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP[] {
	"This setting cannot be changed outside Surface rendering mode."
};

class GraphElement : public UiElement {
private:
	const std::string _title { "Graph" };
	std::array<char, 255> _expressionBuf;

	bool _clampZ;

	GraphFigure* _graph;
	int _cells;

	void expressionInput();
	void gridToggle();
	void renderModeSlider();
	void resolutionInput();
	void renderSettings();

	std::string _expressionInputId;
	std::string _gridToggleId;
	std::string _renderModeId;
	std::string _resolutionInputId;
	std::string _resolutionUpdateId;
	std::string _normalToggleId;
	std::string _clampZToggleId;
	std::string _gpuUploadToggleId;
	std::string _regenerateToggleId;
public:
	const std::string& title() const override { return _title; }
	unsigned int id() const override { return _graph->id(); }
	void show() override;

	GraphElement() = delete;
	GraphElement(GraphFigure& graph)
	: _graph { &graph },
	  _cells { static_cast<int>(graph.cells()) },
	  _clampZ { graph.clampZ() },
	  _expressionInputId { std::format("Expression##{}", graph.id()) },
	  _gridToggleId { std::format("Show Grid##{}", graph.id()) },
	  _renderModeId { std::format("Render Mode##{}", graph.id()) },
	  _resolutionInputId { std::format("Resolution##{}", graph.id()) },
	  _resolutionUpdateId { std::format("Update##{}", graph.id()) },
	  _normalToggleId { std::format("Show Normals##{}", graph.id()) },
	  _clampZToggleId { std::format("Clamp Z##{}", graph.id()) },
	  _gpuUploadToggleId { std::format("GPU Upload##{}", graph.id()) },
	  _regenerateToggleId { std::format("Regenerate##{}", graph.id()) }
	{
		std::fill(_expressionBuf.begin(), _expressionBuf.end(), '\0');
	}
};

template<>
struct UiElementForFigure<GraphFigure> {
	typedef GraphElement value;
};
}
