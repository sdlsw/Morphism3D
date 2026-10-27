#include "ui/element/graph.h"

static const std::string RENDER_MODE_SURFACE_NAME = "Surface";
static const std::string RENDER_MODE_WIREFRAME_NAME = "Wireframe";
static const std::string RENDER_MODE_NONE_NAME = "None";
static const std::string RENDER_MODE_UNKNOWN_NAME { g3d::UI_UNKNOWN_MODE_NAME };

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

const std::string& renderModeName(const g3d::SurfaceGraphRenderMode& mode) {
	switch (mode) {
		case g3d::SurfaceGraphRenderMode::surface:
			return RENDER_MODE_SURFACE_NAME;
		case g3d::SurfaceGraphRenderMode::wireframe:
			return RENDER_MODE_WIREFRAME_NAME;
		case g3d::SurfaceGraphRenderMode::none:
			return RENDER_MODE_NONE_NAME;
		default:
			return RENDER_MODE_UNKNOWN_NAME;
	}
}

namespace g3d {
SurfaceGraphElement::SurfaceGraphElement(SurfaceGraphFigure& graph)
: _graph { &graph },
  _title { std::format("Graph (figID {})", graph.id()) },
  _cells { static_cast<int>(graph.cells()) },
  _clampOut { graph.clampOut() },
  _guiIds { std::format("##{}", graph.id()) },
  _appearanceWindow { graph.id(), graph.appearance(), COLOR_PRESETS }
{
	std::fill(_expressionBuf.begin(), _expressionBuf.end(), '\0');
	_appearanceWindow.open = false;
}

void SurfaceGraphElement::expressionInput() {
	bool changed = ImGui::InputText(
		_guiIds.c_str("Expression"),
		_expressionBuf.data(),
		_expressionBuf.size()
	);
	if (changed) {
		_graph->func().updateExpression(0, _expressionBuf.data());
	}
}

void SurfaceGraphElement::gridToggle() {
	bool disabled = _graph->settings().renderMode != SurfaceGraphRenderMode::surface;
	bool forceSetting = _graph->settings().renderMode == SurfaceGraphRenderMode::wireframe;
	bool* setting = disabled ? &forceSetting : &_graph->settings().renderGrid;

	ImGui::BeginDisabled(disabled);
	ImGui::Checkbox(_guiIds.c_str("Show Grid"), setting);
	if (disabled) ImGui::SetItemTooltip(SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP);
	ImGui::EndDisabled();
}

void SurfaceGraphElement::renderModeSlider() {
	ImGui::SliderInt(
		_guiIds.c_str("Render Mode"),
		reinterpret_cast<int*>(&_graph->settings().renderMode),
		0,
		SurfaceGraphRenderModeCount-1,
		renderModeName(_graph->settings().renderMode).c_str(),
		ImGuiSliderFlags_NoInput
	);
}

void SurfaceGraphElement::resolutionInput() {
	// Maximum safe resolution is about 250 due to use of
	// uint16_t for indices, so always clamp. TODO switch to
	// uint32_t for even higher max resolution?
	ImGui::SliderInt(_guiIds.c_str("Resolution"), &_cells, 10, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SameLine();
	if (ImGui::Button(_guiIds.c_str("Update"))) {
		_graph->cells(static_cast<unsigned int>(_cells));
	}
}

void SurfaceGraphElement::renderSettings() {
	ImGui::SeparatorText("Render Settings");
	resolutionInput();
	renderModeSlider();
	gridToggle();
	ImGui::Checkbox(_guiIds.c_str("Show Normals"), &_graph->settings().renderNormals);
	bool clampChanged = ImGui::Checkbox(_guiIds.c_str("Clamp Out"), &_clampOut);
	if (clampChanged) {
		_graph->clampOut(_clampOut);
	}

	ImGui::Checkbox(
		_guiIds.c_str("Show Appearance Settings"),
		&_appearanceWindow.open
	);
}

void SurfaceGraphElement::show() {
	_appearanceWindow.show();

	expressionInput();
	renderSettings();

	ImGui::SeparatorText("Debug");
	ImGui::Checkbox(_guiIds.c_str("GPU Upload"), &_graph->doUpload);
	ImGui::Checkbox(_guiIds.c_str("Regenerate"), &_graph->doRegen);
}
}
