#include "ui/window/graph_appearance.h"

namespace g3d {
void GraphAppearanceWindow::drawUi() {
	ImGui::SeparatorText("Color");
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
