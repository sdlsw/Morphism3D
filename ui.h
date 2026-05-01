#pragma once

#include "camera_control.h"
#include "vk.h"

#include <string>

namespace g3d {
// Wrapper class for ImGui setup/teardown to make use of C++'s predictable
// destructor behavior. Cannot be moved or copied.
class ImGuiWrapper {
public:
	ImGuiWrapper(const ImGuiWrapper& other) = delete;
	ImGuiWrapper(ImGuiWrapper&& other) = delete;
	ImGuiWrapper& operator=(const ImGuiWrapper& other) = delete;
	ImGuiWrapper& operator=(ImGuiWrapper&& other) = delete;

	ImGuiWrapper(GraphDevice& device, Renderer& renderer);
	~ImGuiWrapper();
};

void imGuiInfo(const std::string& message);
void imGuiNewFrame();
void imGuiRecord(RenderContext& ctx);
void imGuiHandleControlExclusivity(Window& window, CameraController& camController);

class CameraWindow {
private:
	CameraController* _camController;
	void resetButton(const std::string& resetFor, float* setting, float defaultValue);
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
	void alignButtons();
	void controlTutorial();

public:
	bool open = true;
	void show();

	CameraWindow(CameraController& camController) : _camController { &camController } {}
};

class Ui {
private:
	CameraWindow _cameraWindow;

	void mainMenuBar();
	void windowMenu();

public:
	bool showDemoWindow = false;
	void show();

	Ui(CameraController& camController) : _cameraWindow { camController } {}
};
}
