#include "ui/window/graph_appearance.h"

const bool GRAPH_COLOR_SINGLE = false;
const bool GRAPH_COLOR_FOUR = true;

const g3d::GraphColorPreset PRESETS[] {
	{
		"Red",
		GRAPH_COLOR_SINGLE,
		{1.000f, 0.100f, 0.100f}
	},
	{
		"Green",
		GRAPH_COLOR_SINGLE,
		{0.100f, 1.000f, 0.100f}
	},
	{
		"Blue",
		GRAPH_COLOR_SINGLE,
		{0.100f, 0.100f, 1.000f}
	},
	{
		"Color Wheel 1",
		GRAPH_COLOR_FOUR,
		{1.000f, 0.000f, 0.000f},
		{0.500f, 0.000f, 1.000f},
		{0.000f, 1.000f, 1.000f},
		{0.500f, 1.000f, 0.000f}
	},
	{
		"Color Wheel 2",
		GRAPH_COLOR_FOUR,
		{1.000f, 0.000f, 0.500f},
		{0.000f, 0.000f, 1.000f},
		{0.000f, 1.000f, 0.500f},
		{1.000f, 1.000f, 0.000f}
	},
	{
		"Color Wheel 3",
		GRAPH_COLOR_FOUR,
		{1.000f, 0.000f, 1.000f},
		{0.000f, 0.500f, 1.000f},
		{0.000f, 1.000f, 0.000f},
		{1.000f, 0.500f, 0.000f}
	},
	{
		"Classic",
		GRAPH_COLOR_FOUR,
		{0.141f, 0.706f, 0.322f}, // green
		{0.988f, 0.804f, 0.000f}, // yellow orange
		{0.400f, 0.255f, 0.953f}, // blue violet
		{1.000f, 0.000f, 0.000f}  // red
	}
};

namespace g3d {
void GraphAppearanceWindow::loadColorPreset(const GraphColorPreset& preset) {
	if (preset.colorMode) { // Four color
		_appearance->nxnyColor = preset.nxnyColor;
		_appearance->pxnyColor = preset.pxnyColor;
		_appearance->nxpyColor = preset.nxpyColor;
		_appearance->pxpyColor = preset.pxpyColor;
	} else {
		_appearance->setSingleColor(preset.nxnyColor);
	}

	_appearance->colorChanged.set();
	_colorMode = preset.colorMode;
}

void GraphAppearanceWindow::drawUi() {
	ImGui::SeparatorText("Color");
	if(ImGui::Button("Load Preset...")) {
		ImGui::OpenPopup("graph_appearance_load_preset_popup");
	}

	int selectedPreset = -1;
	if (ImGui::BeginPopup("graph_appearance_load_preset_popup")) {
		ImGui::SeparatorText("Presets");
		for (unsigned int i = 0; i < std::size(PRESETS); i++) {
			if (ImGui::Selectable(PRESETS[i].title.c_str())) {
				selectedPreset = static_cast<int>(i);
			}
		}
		ImGui::EndPopup();
	}

	if (selectedPreset >= 0) {
		loadColorPreset(PRESETS[selectedPreset]);
	}

	bool modeChanged = ImGui::Checkbox("Four Colors", &_colorMode);
	if (modeChanged) {
		if (!_colorMode) { // switched to single color mode
			_appearance->setSingleColor(_appearance->nxnyColor);
			_appearance->colorChanged.set();
		}
	}

	if (_colorMode) { // four color mode
		bool nxnyChanged = ImGui::ColorEdit3("nxny", glm::value_ptr(_appearance->nxnyColor));
		bool pxnyChanged = ImGui::ColorEdit3("pxny", glm::value_ptr(_appearance->pxnyColor));
		bool nxpyChanged = ImGui::ColorEdit3("nxpy", glm::value_ptr(_appearance->nxpyColor));
		bool pxpyChanged = ImGui::ColorEdit3("pxpy", glm::value_ptr(_appearance->pxpyColor));

		if (nxnyChanged || pxnyChanged || nxpyChanged || pxpyChanged) {
			_appearance->colorChanged.set();
		}
	} else {
		bool colorChanged = ImGui::ColorEdit3("Color", glm::value_ptr(_appearance->nxnyColor));
		if (colorChanged) {
			_appearance->setSingleColor(_appearance->nxnyColor);
			_appearance->colorChanged.set();
		}
	}

	ImGui::SeparatorText("Material");
	resettableMaterialPanel(_appearance->material);
}
}
