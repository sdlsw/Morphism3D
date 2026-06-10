#include "ui/camera_window.h"

static const std::string CAM_MODE_FIXED_NAME = "Fixed";
static const std::string CAM_MODE_FREE_NAME = "Free";
static const std::string CAM_MODE_UNKNOWN_NAME { g3d::UI_UNKNOWN_MODE_NAME };

static const std::string PERSPECTIVE_FIXED_IN_FREE_TOOLTIP = "This setting is fixed to 1 in Free mode.";
static const std::string POSITION_DISABLED_FIXED_TOOLTIP = "Position is set automatically in Fixed mode.";

static const std::string& camModeName(const g3d::CameraMode& mode) {
	switch (mode) {
		case g3d::CameraMode::fixedLook:
			return CAM_MODE_FIXED_NAME;
		case g3d::CameraMode::forward:
			return CAM_MODE_FREE_NAME;
		default:
			return CAM_MODE_UNKNOWN_NAME;
	}
}

namespace g3d {
void CameraWindow::settingSlider(
	const std::string& label,
	float* setting,
	float defaultValue
) {
	resettableSlider(label, setting, defaultValue, defaultValue / 5.0f, defaultValue * 3.0f);
}

void CameraWindow::projectionSlider() {
	bool disabled = _camController->mode() == CameraMode::forward;

	float fixed = Camera::defaultProjectionMix;
	float* setting = disabled ? &fixed : &_camController->camera().projectionMix;

	ImGui::BeginDisabled(disabled);
	// TODO Should use BeginGroup/EndGroup for this, but it messes with
	// the spacing of the Reset button. Will figure it out later.
	ImGui::SliderFloat("Perspective", setting, 0.0f, 1.0f);
	if (disabled) ImGui::SetItemTooltip(PERSPECTIVE_FIXED_IN_FREE_TOOLTIP.c_str());
	resetButton("Perspective", setting, Camera::defaultProjectionMix);
	if (disabled) ImGui::SetItemTooltip(PERSPECTIVE_FIXED_IN_FREE_TOOLTIP.c_str());
	ImGui::EndDisabled();
}

void CameraWindow::settingSliders() {
	settingSlider(
		"Sensitivity",
		&_camController->sensitivity,
		CameraController::defaultSensitivity
	);
	settingSlider(
		"Freecam Speed",
		&_camController->moveSpeed,
		CameraController::defaultMoveSpeed
	);
	settingSlider(
		"Scroll Speed",
		&_camController->scrollSpeed,
		CameraController::defaultScrollSpeed
	);
}

void CameraWindow::modeSlider() {
	int currentMode = static_cast<int>(_camController->mode());
	int sliderMode = currentMode;

	ImGui::SliderInt(
		"Mode",
		&sliderMode,
		0,
		CameraModeCount-1,
		camModeName(_camController->mode()).c_str(),
		ImGuiSliderFlags_NoInput
	);

	if (sliderMode != currentMode) {
		_camController->mode(static_cast<CameraMode>(sliderMode));
	}
}

void CameraWindow::transformDrags() {
	bool positionDisabled = _camController->mode() == CameraMode::fixedLook;

	ImGui::BeginDisabled(positionDisabled);
	ImGui::DragFloat3(
		"Position",
		glm::value_ptr(_camController->camera().position),
		0.025f,
		-FLT_MAX, FLT_MAX
	);
	if (positionDisabled) {
		ImGui::SetItemTooltip(POSITION_DISABLED_FIXED_TOOLTIP.c_str());
	}
	ImGui::EndDisabled();

	// Note: No need to limit angle here since the camera will apply modulo
	// 2pi on update.
	ImGui::DragFloat2(
		"Angles",
		glm::value_ptr(_camController->camera().angles),
		0.025f,
		-FLT_MAX, FLT_MAX
	);
}

void CameraWindow::alignButtons() {
	auto fullCircle = 2.0f * glm::pi<float>();
	std::vector<std::pair<std::string, glm::vec2>> axes {
		{"+X", {fullCircle * 0.75f, 0.0f}},
		{"-X", {fullCircle * 0.25f, 0.0f}},
		{"+Y", {0.0f, 0.0f}},
		{"-Y", {fullCircle * 0.5f, 0.0f}},
		{"+Z", {fullCircle * 0.5f, fullCircle * 0.25f}},
		{"-Z", {0.0f, fullCircle * 0.75f}}
	};

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Align");
	for (const auto& [label, angles] : axes) {
		ImGui::SameLine();
		if (ImGui::Button(label.c_str())) {
			_camController->camera().angles = angles;
		}
	}
}

void CameraWindow::controlTutorial() {
	if (ImGui::TreeNode("Controls")) {
		ImGui::Text("Click outside a window to capture the cursor.");
		ImGui::Text("ESC - Free cursor");
		ImGui::Text("E - Toggle camera mode");
		ImGui::Text("R - Reset camera view");

		if (_camController->mode() == CameraMode::fixedLook) {
			ImGui::Text("Scroll wheel - Up zooms in, down zooms out");
			ImGui::Text("Move mouse - Rotate around graph");
		} else {
			ImGui::Text("WASD - Standard first person controls");
			ImGui::Text("Space - Move up");
			ImGui::Text("Left Shift - Move down");
			ImGui::Text("Move mouse - Look around");
		}

		ImGui::TreePop();
	}
}

void CameraWindow::drawUi() {
	ImGui::SeparatorText("Settings");
	settingSliders();
	projectionSlider();
	modeSlider();
	if (ImGui::Button("Reset View")) {
		_camController->reset();
	}

	ImGui::SeparatorText("Transform");
	transformDrags();
	alignButtons();

	ImGui::SeparatorText("Info");
	controlTutorial();
}
}
