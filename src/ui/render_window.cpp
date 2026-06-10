#include "ui/render_window.h"

namespace g3d {
void RenderWindow::basicSettingsSection() {
	ImGui::SeparatorText("Basic Settings");

	ImGui::Checkbox("Show Axes", &_renderSettings->renderAxes);
	ImGui::Checkbox("Show Frame", &_renderSettings->renderFrame);
	ImGui::Checkbox("Show Light Position", &_renderSettings->renderLightObject);
}

void RenderWindow::lightSection() {
	ImGui::SeparatorText("Light");
	resettableSlider("Mix",
		&_light->current.mix,
		_light->initial().mix,
		0.0f, 1.0f
	);
	resettableSlider("Color",
		&_light->current.color,
		_light->initial().color,
		0.0f, 1.0f
	);
	resettableDrag("Position",
		&_light->current.position,
		_light->initial().position,
		0.025f
	);
	resetAllButton("Light", *_light);
}

void RenderWindow::materialSection() {
	ImGui::SeparatorText("Material");
	resettableSlider("Ambient",
		&_material->current.ambient,
		_material->initial().ambient,
		0.0f, 1.0f
	);
	resettableSlider("Diffuse",
		&_material->current.diffuse,
		_material->initial().diffuse,
		0.0f, 1.0f
	);
	resettableSlider("Specular",
		&_material->current.specular,
		_material->initial().specular,
		0.0f, 1.0f
	);
	resettableSlider("Shine",
		&_material->current.shine,
		_material->initial().shine,
		1.0f, 64.0f
	);
	resetAllButton("Material", *_material);
}

void RenderWindow::drawUi() {
	basicSettingsSection();
	lightSection();
	materialSection();
}
}
