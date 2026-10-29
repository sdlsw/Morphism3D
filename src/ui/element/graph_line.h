#pragma once

#include "figure/graph_line.h"
#include "figure/figure.h"
#include "ui/common_graph.h"

namespace g3d {
class LineGraphAppearanceWindow : public UiWindow {
private:
	std::string _title;
	GraphColorPanel<1> _colorPanel;

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	LineGraphAppearanceWindow(
		unsigned int id,
		GraphColors<1>& colors,
		std::span<const GraphColorPreset<1>> presets
	)
	: _title { std::format("Graph (figID {}) Appearance", id) },
	  _colorPanel { colors, presets }
	{}
};

class LineGraphElement : public UiElement {
private:
	const std::string _title;
	LineGraphFigure* _graph;
	DiscriminatedStringMap _guiIds;

	GraphFunctionPanel<1, 2> _functionPanel;
	GraphCommonPanels<LineGraphFigure> _commonPanels;
	LineGraphAppearanceWindow _appearanceWindow;

public:
	const std::string& title() const override { return _title; }
	unsigned int id() const override { return _graph->id(); }
	void show() override;

	LineGraphElement() = delete;
	LineGraphElement(LineGraphFigure& graph);
};

template<>
struct UiElementForFigure<LineGraphFigure> {
	typedef LineGraphElement value;
};
}
