#pragma once

#include "camera_control.h"
#include "container.h"
#include "expression.h"
#include "graph.h"
#include "vk/renderer.h"
#include "window.h"

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

struct VariableSlider {
	// Tests whether a character can be used as a slider.
	// TODO Factor this out? Will need to change when other types of graphs
	// get added.
	static bool varValid(char c);

	// Text buffer used for variable entry
	static constexpr size_t entrySize = 2;
	std::array<char, entrySize> entryBuffer;

	// Every slider gets a unique numeric ID to differentiate it from the
	// others.
	unsigned int id;
	float value;
	float min;
	float max;
	bool exists = true; // Set to false when the slider is closed
	bool changed = false;

	// Flags for the up/down arrows
	bool upPressed = false;
	bool downPressed = false;

	// For convenience, this variable is stored in the slider,
	// but the actual logic is done in SliderPanel.
	unsigned int height = 0;

	// IDs for each of the ImGui widgets
	std::string varEntryId;
	std::string valueEntryId;
	std::string minEntryId;
	std::string maxEntryId;
	std::string upArrowId;
	std::string downArrowId;

	std::string headerId() { return std::format("{}##{}header", var(), id); }

	VariableSlider(unsigned int id, char var, float value, float min, float max)
	: id { id },
	  varEntryId { std::format("##{}varEntry", id) },
	  valueEntryId { std::format("##{}valueEntry", id) },
	  minEntryId { std::format("##{}minEntry", id) },
	  maxEntryId { std::format("##{}maxEntry", id) },
	  upArrowId { std::format("##{}up", id) },
	  downArrowId { std::format("##{}down", id) },
	  entryBuffer { var, '\0' },
	  value { value },
	  min { min },
	  max { max }
	{}

	VariableSlider(unsigned int id, char var) : VariableSlider(id, var, 1.0f, 0.0f, 3.0f) {}
	VariableSlider(unsigned int id) : VariableSlider(id, '\0') {}

	// Show logic split into two functions for imgui group stuff in SliderPanel
	bool showHeader();
	void showBody();

	// Gets the variable this slider is connected to
	char var() const;
};

class SliderPanel {
private:
	Window* _window;
	VariableStore* _vars;

	std::unordered_map<unsigned int, VariableSlider> _sliders;
	std::vector<unsigned int> _sliderOrder;
	unsigned int _nextSliderId = 0;

	bool _changedVar = false;

	size_t _rearrangeFrom = 0;
	size_t _rearrangeTo = 0;
	bool _rearrange = false;

	// Need to do high level slider show logic here since drag/drop
	// interacts with _sliderOrder.
	void showSlider(size_t i);

	// Shows all sliders, handles delete/variable update logic
	void showSliders();

	// Utility function. Gets the i'th slider based on order in the UI
	VariableSlider& getSlider(size_t i);

	// Returns true if this panel has a slider with the given character
	// defined.
	bool hasSlider(char c);

	// Adds a slider. Sliders are removed in the UI through an X on their
	// header bar.
	void addSlider(char c);

	// Finds an available variable that doesn't already have a slider.
	// Returns '\0' on failure.
	char findFirstAvailableVar();
public:
	SliderPanel(Window& window, VariableStore& vars)
	: _window { &window }, _vars { &vars } {}
	void show();
	bool consumeChangedFlag();
};

template<typename T>
class GraphWindow : public UiWindow {
private:
	const std::string _title { "Graph" };
	std::array<char, 255> _expressionBuf;

	glm::vec3 _rangeLow;
	glm::vec3 _rangeHigh;

	Graph<T>* _graph;
	int _cells;

	SliderPanel _sliders;

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

	void rangeInput(char dim, float& low, float& high) {
		float lastLow = low;
		bool lowChanged = ImGui::InputFloat(std::format("##rangelow{}", dim).c_str(), &low);
		if (lowChanged) {
			if (low >= high) {
				low = lastLow;
			} else {
				_graph->rangeLow(_rangeLow);
			}
		}

		char dimStr[] { dim, '\0' };
		ImGui::SameLine();
		ImGui::Text("<=");
		ImGui::SameLine();
		ImGui::Text(dimStr);
		ImGui::SameLine();
		ImGui::Text("<=");
		ImGui::SameLine();

		float lastHigh = high;
		bool highChanged = ImGui::InputFloat(std::format("##rangehigh{}", dim).c_str(), &high);
		if (highChanged) {
			if (high <= low) {
				high = lastHigh;
			} else {
				_graph->rangeHigh(_rangeHigh);
			}
		}
	}

	void rangeInputs() {
		ImGui::SeparatorText("Range");
		char dims[] { 'X', 'Y', 'Z' };

		ImGui::PushItemWidth(100.0f);
		for (unsigned int i = 0; i < 3; i++) {
			rangeInput(dims[i], _rangeLow[i], _rangeHigh[i]);
		}
		ImGui::PopItemWidth();
	}

	void sliders() {
		ImGui::SeparatorText("Sliders");
		_sliders.show();
		if (_sliders.consumeChangedFlag()) {
			_graph->func().setUpdated();
		}
	}

public:
	const std::string& title() const override { return _title; }
	void drawUi() override {
		expressionInput();

		rangeInputs();

		ImGui::SeparatorText("Render Settings");
		resolutionInput();
		renderModeSlider();
		gridToggle();
		ImGui::Checkbox("Show Normals", &_graph->renderNormals);

		ImGui::SeparatorText("Debug");
		ImGui::Checkbox("GPU Upload", &_graph->doUpload);
		ImGui::Checkbox("Regenerate", &_graph->doRegen);

		sliders();
	}

	GraphWindow() = delete;
	GraphWindow(Graph<T>& graph, Window& window)
	: _graph { &graph },
	  _cells { static_cast<int>(graph.cells()) },
	  _sliders { window, graph.func().vars() },
	  _rangeLow { graph.rangeLow() },
	  _rangeHigh { graph.rangeHigh() }
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
