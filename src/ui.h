#pragma once

#include "camera_control.h"
#include "container.h"
#include "vk/renderer.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

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
void imGuiRecord(Renderer& renderer);
void imGuiHandleControlExclusivity(Window& window, CameraController& camController);

template<typename T>
void resetAllButton(const std::string& name, WithInitial<T>& withInitial) {
	if(ImGui::Button(std::format("Reset All##{}", name).c_str())) {
		withInitial.reset();
	}
}

template<typename T>
void resetButton(const std::string& label, T* setting, T _default) {
	ImGui::SameLine(ImGui::GetWindowWidth()-50);
	if (ImGui::Button(std::format("Reset##{}", label).c_str())) {
		*setting = _default;
	}
}

template<typename T, typename U>
void resettableSlider(const std::string& label, T* setting, const T& _default, U min, U max);

template<typename T, typename U>
void resettableDrag(const std::string& label, T* setting, const T& _default, U inc);

class CameraWindow {
private:
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
	bool open = true;
	void show();

	CameraWindow() = delete;
	CameraWindow(CameraController& camController) : _camController { &camController } {}
};

class RenderWindow {
private:
	RenderSettings* _renderSettings;
	WithInitial<Light>* _light;
	WithInitial<Material>* _material;

	void showGridToggle();
	void basicSettingsSection();
	void lightSection();
	void materialSection();
public:
	bool open = true;
	void show();

	RenderWindow() = delete;
	RenderWindow(
		RenderSettings& renderSettings,
		WithInitial<Light>& light,
		WithInitial<Material>& material
	)
	: _renderSettings { &renderSettings },
	  _light { &light },
	  _material { &material }
	{}
};

class DebugWindow {
private:
	DebugSettings* _debugSettings;

public:
	bool open = false;
	void show();

	DebugWindow() = delete;
	DebugWindow(DebugSettings& debugSettings)
	: _debugSettings { &debugSettings }
	{}
};

class Ui {
private:
	CameraWindow _cameraWindow;
	RenderWindow _renderWindow;
	DebugWindow _debugWindow;

	void mainMenuBar();
	void windowMenu();

public:
	bool showDemoWindow = false;
	void show();

	Ui(
		CameraController& camController,
		RenderSettings& renderSettings,
		DebugSettings& debugSettings,
		WithInitial<Light>& light,
		WithInitial<Material>& material
	)
	: _cameraWindow { camController },
	  _debugWindow { debugSettings },
	  _renderWindow { renderSettings, light, material }
	{}
};
}
