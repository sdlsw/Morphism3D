#pragma once

#include "camera_control.h"
#include "container.h"
#include "vk_renderer.h"

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
	void resetButton(float* setting, float defaultValue);
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
	bool open = true;
	void show();

	CameraWindow() = delete;
	CameraWindow(CameraController& camController) : _camController { &camController } {}
};

class RenderWindow {
private:
	RenderSettings* _renderSettings;

public:
	bool open = true;
	void show();

	RenderWindow() = delete;
	RenderWindow(RenderSettings& renderSettings) : _renderSettings { &renderSettings } {}
};

class Ui {
private:
	CameraWindow _cameraWindow;
	RenderWindow _renderWindow;

	void mainMenuBar();
	void windowMenu();

public:
	bool showDemoWindow = false;
	void show();

	Ui(
		CameraController& camController,
		RenderSettings& renderSettings
	)
	: _cameraWindow { camController },
	  _renderWindow { renderSettings }
	{}
};
}
