#pragma once

#include "camera_control.h"
#include "container.h"
#include "graph.h"
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

// Unfortunately need to define this in header since it's used in a template.
constexpr const char SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP[] {
	"This setting cannot be changed outside Surface rendering mode."
};

const std::string& renderModeName(const GraphRenderMode& mode);

class UiWindow {
private:
	const std::string unknownTitle { "unknown" };
public:
	bool open = true;
	void show();
	virtual const std::string& title() const { return unknownTitle; }
	virtual void drawUi() {}
};

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

class RenderWindow : public UiWindow {
private:
	const std::string _title { "Render" };

	RenderSettings* _renderSettings;
	WithInitial<Light>* _light;
	WithInitial<Material>* _material;

	void basicSettingsSection();
	void lightSection();
	void materialSection();
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

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

template<typename T>
class GraphWindow : public UiWindow {
private:
	const std::string _title { "Graph" };
	std::array<char, 255> _expressionBuf;

	Graph<T>* _graph;
	int _cells;

	void expressionInput() {
		bool changed = ImGui::InputText("Expression", _expressionBuf.data(), _expressionBuf.size());
		if (changed) {
			_graph->func().updateExpression(_expressionBuf.data());
		}
	}

	void gridToggle() {
		bool disabled = _graph->renderMode != GraphRenderMode::surface;
		bool forceSetting = _graph->renderMode == GraphRenderMode::wireframe;
		bool* setting = disabled ? &forceSetting : &_graph->renderGrid;

		ImGui::BeginDisabled(disabled);
		ImGui::Checkbox("Show Grid", setting);
		if (disabled) ImGui::SetItemTooltip(SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP);
		ImGui::EndDisabled();
	}

	void renderModeSlider() {
		ImGui::SliderInt(
			"Render Mode",
			reinterpret_cast<int*>(&_graph->renderMode),
			0,
			GraphRenderModeCount-1,
			renderModeName(_graph->renderMode).c_str(),
			ImGuiSliderFlags_NoInput
		);
	}

	void resolutionInput() {
		// Maximum safe resolution is about 250 due to use of
		// uint16_t for indices, so always clamp. TODO switch to
		// uint32_t for even higher max resolution?
		ImGui::SliderInt("Resolution", &_cells, 10, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SameLine();
		if (ImGui::Button("Update")) {
			_graph->cells(static_cast<unsigned int>(_cells));
		}
	}
public:
	const std::string& title() const override { return _title; }
	void drawUi() override {
		expressionInput();

		ImGui::SeparatorText("Render Settings");
		resolutionInput();
		renderModeSlider();
		gridToggle();
		ImGui::Checkbox("Show Normals", &_graph->renderNormals);

		ImGui::SeparatorText("Debug");
		ImGui::Checkbox("GPU Upload", &_graph->doUpload);
		ImGui::Checkbox("Regenerate", &_graph->doRegen);
	}

	GraphWindow() = delete;
	GraphWindow(Graph<T>& graph)
	: _graph { &graph },
	  _cells { static_cast<int>(graph.cells()) }
	{
		std::fill(_expressionBuf.begin(), _expressionBuf.end(), '\0');
	}
};

class DebugWindow : public UiWindow {
private:
	const std::string _title { "Debug" };

	DebugSettings* _debugSettings;

	void tokenizerTestInput();
	void parserTestInput();
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	DebugWindow() = delete;
	DebugWindow(DebugSettings& debugSettings)
	: _debugSettings { &debugSettings }
	{
		open = false;
	}
};

class Ui {
private:
	std::vector<std::unique_ptr<UiWindow>> _windows;

	void mainMenuBar();
	void windowMenu();

public:
	bool showDemoWindow = false;
	void show();

	Ui() = default;
	Ui(Ui&&) = delete;
	Ui(const Ui&) = delete;

	template<std::derived_from<UiWindow> W, typename... Args>
	void addWindow(Args&&... args) {
		auto window = std::make_unique<W>(std::forward<Args>(args)...);
		_windows.emplace_back(std::move(window));
	}
};
}
