#include "ui/element/graph.h"

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
void GraphElement::expressionInput() {
	bool changed = ImGui::InputText(
		_guiIds.c_str("Expression"),
		_expressionBuf.data(),
		_expressionBuf.size()
	);
	if (changed) {
		_graph->func().updateExpression(_expressionBuf.data());
	}
}

void GraphElement::gridToggle() {
	bool disabled = _graph->renderMode != GraphRenderMode::surface;
	bool forceSetting = _graph->renderMode == GraphRenderMode::wireframe;
	bool* setting = disabled ? &forceSetting : &_graph->renderGrid;

	ImGui::BeginDisabled(disabled);
	ImGui::Checkbox(_guiIds.c_str("Show Grid"), setting);
	if (disabled) ImGui::SetItemTooltip(SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP);
	ImGui::EndDisabled();
}

void GraphElement::renderModeSlider() {
	ImGui::SliderInt(
		_guiIds.c_str("Render Mode"),
		reinterpret_cast<int*>(&_graph->renderMode),
		0,
		GraphRenderModeCount-1,
		renderModeName(_graph->renderMode).c_str(),
		ImGuiSliderFlags_NoInput
	);
}

void GraphElement::resolutionInput() {
	// Maximum safe resolution is about 250 due to use of
	// uint16_t for indices, so always clamp. TODO switch to
	// uint32_t for even higher max resolution?
	ImGui::SliderInt(_guiIds.c_str("Resolution"), &_cells, 10, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
	ImGui::SameLine();
	if (ImGui::Button(_guiIds.c_str("Update"))) {
		_graph->cells(static_cast<unsigned int>(_cells));
	}
}

void GraphElement::renderSettings() {
	ImGui::SeparatorText("Render Settings");
	resolutionInput();
	renderModeSlider();
	gridToggle();
	ImGui::Checkbox(_guiIds.c_str("Show Normals"), &_graph->renderNormals);
	bool clampChanged = ImGui::Checkbox(_guiIds.c_str("Clamp Z"), &_clampZ);
	if (clampChanged) {
		_graph->clampZ(_clampZ);
	}

	ImGui::Checkbox(
		_guiIds.c_str("Show Appearance Settings"),
		&_appearanceWindow.open
	);
}

void GraphElement::show() {
	_appearanceWindow.show();

	expressionInput();
	renderSettings();

	ImGui::SeparatorText("Debug");
	ImGui::Checkbox(_guiIds.c_str("GPU Upload"), &_graph->doUpload);
	ImGui::Checkbox(_guiIds.c_str("Regenerate"), &_graph->doRegen);
}
}
