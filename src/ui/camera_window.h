#pragma once
#include "ui/common.h"

#include "camera_control.h"

#include <string>

namespace g3d {
class CameraWindow : public UiWindow {
private:
	const std::string _title { "Camera" };

	CameraController* _camController;
	void settingSlider(
		const std::string& label,
		float* setting,
		float defaultValue,
		float minValue,
		float maxValue
	);
	void settingSlider(
		const std::string& label,
		float* setting,
		float defaultValue
	);
	void settingSliders();
	void projectionSlider();
	void modeSlider();
	void transformDrags();
	void alignButtons();
	void controlTutorial();

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	CameraWindow() = delete;
	CameraWindow(CameraController& camController) : _camController { &camController } {}
};
}
