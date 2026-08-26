#include "ui/window/graph.h"

static const std::string RENDER_MODE_SURFACE_NAME = "Surface";
static const std::string RENDER_MODE_WIREFRAME_NAME = "Wireframe";
static const std::string RENDER_MODE_NONE_NAME = "None";
static const std::string RENDER_MODE_UNKNOWN_NAME { g3d::UI_UNKNOWN_MODE_NAME };

const std::string& renderModeName(const g3d::GraphRenderMode& mode) {
	switch (mode) {
		case g3d::GraphRenderMode::surface:
			return RENDER_MODE_SURFACE_NAME;
		case g3d::GraphRenderMode::wireframe:
			return RENDER_MODE_WIREFRAME_NAME;
		case g3d::GraphRenderMode::none:
			return RENDER_MODE_NONE_NAME;
		default:
			return RENDER_MODE_UNKNOWN_NAME;
	}
}
namespace g3d {
void GraphWindow::expressionInput() {
	bool changed = ImGui::InputText("Expression", _expressionBuf.data(), _expressionBuf.size());
	if (changed) {
		_graph->func().updateExpression(_expressionBuf.data());
	}
}

void GraphWindow::gridToggle() {
	bool disabled = _graph->renderMode != GraphRenderMode::surface;
	bool forceSetting = _graph->renderMode == GraphRenderMode::wireframe;
	bool* setting = disabled ? &forceSetting : &_graph->renderGrid;

	ImGui::BeginDisabled(disabled);
	ImGui::Checkbox("Show Grid", setting);
	if (disabled) ImGui::SetItemTooltip(SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP);
	ImGui::EndDisabled();
}

void GraphWindow::renderModeSlider() {
	ImGui::SliderInt(
		"Render Mode",
		reinterpret_cast<int*>(&_graph->renderMode),
		0,
		GraphRenderModeCount-1,
		renderModeName(_graph->renderMode).c_str(),
		ImGuiSliderFlags_NoInput
	);
}

void GraphWindow::resolutionInput() {
	// Maximum safe resolution is about 250 due to use of
	// uint16_t for indices, so always clamp. TODO switch to
	// uint32_t for even higher max resolution?
	ImGui::SliderInt("Resolution", &_cells, 10, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SameLine();
	if (ImGui::Button("Update")) {
		_graph->cells(static_cast<unsigned int>(_cells));
	}
}

void GraphWindow::renderSettings() {
	ImGui::SeparatorText("Render Settings");
	resolutionInput();
	renderModeSlider();
	gridToggle();
	ImGui::Checkbox("Show Normals", &_graph->renderNormals);
	bool clampChanged = ImGui::Checkbox("Clamp Z", &_clampZ);
	if (clampChanged) {
		_graph->clampZ(_clampZ);
	}
}

void GraphWindow::drawUi() {
	expressionInput();
	renderSettings();

	ImGui::SeparatorText("Debug");
	ImGui::Checkbox("GPU Upload", &_graph->doUpload);
	ImGui::Checkbox("Regenerate", &_graph->doRegen);
}
}
