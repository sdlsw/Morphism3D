#include "ui/window/render.h"

namespace g3d {
void RenderWindow::basicSettingsSection() {
	ImGui::SeparatorText("Basic Settings");

	ImGui::Checkbox("Show Axes", &_renderSettings->renderAxes);
	ImGui::Checkbox("Show Frame", &_renderSettings->renderFrame);
	ImGui::Checkbox("Show Light Position", &_renderSettings->renderLightObject);
}

void RenderWindow::lightSection() {
	ImGui::SeparatorText("Light");
	resettableLightPanel(*_light);
}

void RenderWindow::drawUi() {
	basicSettingsSection();
	lightSection();
}
}
