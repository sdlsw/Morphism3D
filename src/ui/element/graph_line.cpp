#include "ui/element/graph_line.h"

const std::array<g3d::GraphColorPreset<1>, 1> COLOR_PRESETS {{
	{
		.title = "Classic",
		.mode = g3d::GRAPH_COLOR_MULTI,
		.multiColor = {{
			{0.141f, 0.706f, 0.322f}, // green
			{1.000f, 0.000f, 0.000f}  // red
		}}
	}
}};

namespace g3d {
void LineGraphAppearanceWindow::drawUi() {
	ImGui::SeparatorText("Color");
	_colorPanel.show();
}

LineGraphElement::LineGraphElement(LineGraphFigure& graph)
: _graph { &graph },
  _title { std::format("Graph (figId {})" , graph.id()) },
  _guiIds { std::format("##{}", graph.id()) },
  _functionPanel { graph.builder().func(), _guiIds },
  _commonPanels { graph, _guiIds, 16000 },
  _appearanceWindow { graph.id(), graph.builder().colors(), COLOR_PRESETS }
{
	_appearanceWindow.open = false;
}

void LineGraphElement::show() {
	_appearanceWindow.show();

	_functionPanel.show();
	ImGui::SeparatorText("Render Settings");
	_commonPanels.showResolutionInput();
	_commonPanels.showClampOutCheckbox();
	_commonPanels.showDebugSettings();
}
}
