#pragma once

#include "ui/common.h"
#include "expression.h"
#include <graph.h>

namespace g3d {
// Unfortunately need to define this in header since it's used in a template.
constexpr const char SHOW_GRID_FIXED_NOT_SURFACE_TOOLTIP[] {
	"This setting cannot be changed outside Surface rendering mode."
};

const std::string& renderModeName(const GraphRenderMode& mode);

template<typename T>
class GraphWindow : public UiWindow {
private:
	const std::string _title { "Graph" };
	std::array<char, 255> _expressionBuf;

	glm::vec3 _rangeLow;
	glm::vec3 _rangeHigh;
	bool _clampZ;

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

	void renderSettings() {
		ImGui::SeparatorText("Render Settings");
		resolutionInput();
		renderModeSlider();
		gridToggle();
		ImGui::Checkbox("Show Normals", &_graph->renderNormals);
		bool clampChanged = ImGui::Checkbox("Clamp Z", &_clampZ);
		if (clampChanged) {
			_graph->clampZ(_clampZ);
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

public:
	const std::string& title() const override { return _title; }
	void drawUi() override {
		expressionInput();
		rangeInputs();
		renderSettings();

		ImGui::SeparatorText("Debug");
		ImGui::Checkbox("GPU Upload", &_graph->doUpload);
		ImGui::Checkbox("Regenerate", &_graph->doRegen);
	}

	GraphWindow() = delete;
	GraphWindow(Graph<T>& graph, Window& window)
	: _graph { &graph },
	  _cells { static_cast<int>(graph.cells()) },
	  _rangeLow { graph.rangeLow() },
	  _rangeHigh { graph.rangeHigh() },
	  _clampZ { graph.clampZ() }
	{
		std::fill(_expressionBuf.begin(), _expressionBuf.end(), '\0');
	}
};
}
