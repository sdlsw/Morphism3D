#include "ui/element/graph_surface.h"

static const std::string RENDER_MODE_SURFACE_NAME = "Surface";
static const std::string RENDER_MODE_WIREFRAME_NAME = "Wireframe";
static const std::string RENDER_MODE_NONE_NAME = "None";
static const std::string RENDER_MODE_UNKNOWN_NAME { g3d::UI_UNKNOWN_MODE_NAME };

static const std::string SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP {
	"This setting cannot be changed outside Surface rendering mode."
};

const std::array<g3d::GraphColorPreset<2>, 7> COLOR_PRESETS {{
	{
		.title = "Red",
		.mode = g3d::GRAPH_COLOR_SINGLE,
		.singleColor = {1.000f, 0.100f, 0.100f}
	},
	{
		.title = "Green",
		.mode = g3d::GRAPH_COLOR_SINGLE,
		.singleColor = {0.100f, 1.000f, 0.100f}
	},
	{
		.title = "Blue",
		.mode = g3d::GRAPH_COLOR_SINGLE,
		.singleColor = {0.100f, 0.100f, 1.000f}
	},
	{
		.title = "Color Wheel 1",
		.mode = g3d::GRAPH_COLOR_MULTI,
		.multiColor = {{
			{1.000f, 0.000f, 0.000f},
			{0.500f, 0.000f, 1.000f},
			{0.000f, 1.000f, 1.000f},
			{0.500f, 1.000f, 0.000f}
		}}
	},
	{
		.title = "Color Wheel 2",
		.mode = g3d::GRAPH_COLOR_MULTI,
		.multiColor = {{
			{1.000f, 0.000f, 0.500f},
			{0.000f, 0.000f, 1.000f},
			{0.000f, 1.000f, 0.500f},
			{1.000f, 1.000f, 0.000f}
		}}
	},
	{
		.title = "Color Wheel 3",
		.mode = g3d::GRAPH_COLOR_MULTI,
		.multiColor = {{
			{1.000f, 0.000f, 1.000f},
			{0.000f, 0.500f, 1.000f},
			{0.000f, 1.000f, 0.000f},
			{1.000f, 0.500f, 0.000f}
		}}
	},
	{
		.title = "Classic",
		.mode = g3d::GRAPH_COLOR_MULTI,
		.multiColor = {{
			{0.141f, 0.706f, 0.322f}, // green
			{0.988f, 0.804f, 0.000f}, // yellow orange
			{0.400f, 0.255f, 0.953f}, // blue violet
			{1.000f, 0.000f, 0.000f}  // red
		}}
	}
}};

const std::string& renderModeName(const g3d::SurfaceDisplayMode& mode) {
	switch (mode) {
		case g3d::SurfaceDisplayMode::surface:
			return RENDER_MODE_SURFACE_NAME;
		case g3d::SurfaceDisplayMode::wireframe:
			return RENDER_MODE_WIREFRAME_NAME;
		case g3d::SurfaceDisplayMode::none:
			return RENDER_MODE_NONE_NAME;
		default:
			return RENDER_MODE_UNKNOWN_NAME;
	}
}

namespace g3d {
void SurfaceGraphAppearanceWindow::drawUi() {
	ImGui::SeparatorText("Color");
	_colorPanel.show();
	ImGui::SeparatorText("Material");
	resettableMaterialPanel(*_material);
}

SurfaceGraphElement::SurfaceGraphElement(SurfaceGraphFigure& graph)
: _graph { &graph },
  _title { std::format("Graph (figID {})", graph.id()) },
  _guiIds { std::format("##{}", graph.id()) },
  _functionPanel { graph.builder().func(), _guiIds },
  _commonPanels { graph, _guiIds, 250 },
  _appearanceWindow { graph.id(), graph.builder().colors(), graph.display().settings.material, COLOR_PRESETS }
{
	_appearanceWindow.open = false;
}

void SurfaceGraphElement::gridToggle() {
	auto& settings = _graph->display().settings;

	bool disabled = settings.renderMode != SurfaceDisplayMode::surface;
	bool forceSetting = settings.renderMode == SurfaceDisplayMode::wireframe;
	bool* setting = disabled ? &forceSetting : &settings.renderGrid;

	ImGui::BeginDisabled(disabled);
	ImGui::Checkbox(_guiIds.c_str("Show Grid"), setting);
	if (disabled) ImGui::SetItemTooltip(SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP.c_str());
	ImGui::EndDisabled();
}

void SurfaceGraphElement::renderModeSlider() {
	auto& settings = _graph->display().settings;

	ImGui::SliderInt(
		_guiIds.c_str("Render Mode"),
		reinterpret_cast<int*>(&settings.renderMode),
		0,
		SurfaceDisplayModeCount-1,
		renderModeName(settings.renderMode).c_str(),
		ImGuiSliderFlags_NoInput
	);
}

void SurfaceGraphElement::renderSettings() {
	ImGui::SeparatorText("Render Settings");
	_commonPanels.showResolutionInput();
	renderModeSlider();
	gridToggle();
	ImGui::Checkbox(_guiIds.c_str("Show Normals"), &_graph->display().settings.renderNormals);
	_commonPanels.showClampOutCheckbox();

	ImGui::Checkbox(
		_guiIds.c_str("Show Appearance Settings"),
		&_appearanceWindow.open
	);
}

void SurfaceGraphElement::show() {
	_appearanceWindow.show();

	_functionPanel.show();
	renderSettings();
	_commonPanels.showDebugSettings();
}
}
