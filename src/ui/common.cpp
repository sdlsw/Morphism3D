#include "ui/common.h"

namespace g3d {
template<>
void resettableSlider<float, float>(
	const std::string& label,
	float* setting,
	const float& _default,
	float min,
	float max
) {
	ImGui::SliderFloat(label.c_str(), setting, min, max);
	resetButton(label, setting, _default);
}

template<>
void resettableSlider<glm::vec3, float>(
	const std::string& label,
	glm::vec3* setting,
	const glm::vec3& _default,
	float min,
	float max
) {
	ImGui::SliderFloat3(label.c_str(), glm::value_ptr(*setting), min, max);
	resetButton(label, setting, _default);
}

template<>
void resettableDrag<glm::vec3, float>(
	const std::string& label,
	glm::vec3* setting,
	const glm::vec3& _default,
	float inc
) {
	ImGui::DragFloat3(label.c_str(), glm::value_ptr(*setting), inc);
	resetButton(label, setting, _default);
}

void UiWindow::show() {
	if (!open) return;

	ImGui::Begin(title().c_str(), &open);
	ImGui::PushItemWidth(200.0f);
	drawUi();
	ImGui::PopItemWidth();
	ImGui::End();
}
}
