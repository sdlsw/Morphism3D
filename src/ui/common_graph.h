#pragma once

#include "figure/graph_common.h"
#include "ui/common_render.h"

namespace g3d {
template<size_t DIMS>
struct GraphColorPreset {
	std::string title;
	bool mode;

	union {
		glm::vec3 singleColor;
		std::array<glm::vec3, GraphColors<DIMS>::EndpointType::spanSize> multiColor;
	};
};

constexpr bool GRAPH_COLOR_SINGLE = false;
constexpr bool GRAPH_COLOR_MULTI = true;

template<size_t DIMS>
std::string createColorLabel(size_t colorIndex) {
	std::string out;

	for (size_t i = 0; i < DIMS; i++) {
		size_t dimBit = colorIndex & 0x1;
		colorIndex >>= 1;

		// TODO: Pass the var chars in as an array
		char varC = 'x' + static_cast<char>(i);
		out.push_back(dimBit == 0 ? '-' : '+');
		out.push_back(varC);
	}

	return out;
}

template<size_t DIMS>
class GraphColorPanel {
private:
	GraphColors<DIMS>* _colors;
	bool _colorMode { GRAPH_COLOR_MULTI };

	const GraphColorPreset<DIMS>* _presets;
	size_t _numPresets;

	CachedStringFunction<size_t> _colorLabels { &createColorLabel<DIMS> };

	void loadColorPreset(const GraphColorPreset<DIMS>& preset);

public:
	// Note: It is acceptable to use a vector to contain the presets, so
	// long as that vector does not change size once this window is
	// constructed.
	GraphColorPanel(
		GraphColors<DIMS>& colors,
		std::span<const GraphColorPreset<DIMS>> presets
	)
	: _colors { &colors },
	  _presets { presets.data() },
	  _numPresets { presets.size() }
	{}

	void show();
};

template<size_t DIMS>
void GraphColorPanel<DIMS>::loadColorPreset(const GraphColorPreset<DIMS>& preset) {
	if (preset.mode == GRAPH_COLOR_MULTI) {
		_colors->gradientEndpoints.copyFromSpan(preset.multiColor);
	} else {
		_colors->setSingleColor(preset.singleColor);
	}

	_colors->colorChanged.set();
	_colorMode = preset.mode;
}

template<size_t DIMS>
void GraphColorPanel<DIMS>::show() {
	if (ImGui::Button("Load Preset...")) {
		ImGui::OpenPopup("graph_colors_load_preset_popup");
	}

	int selectedPreset = -1;
	if (ImGui::BeginPopup("graph_colors_load_preset_popup")) {
		ImGui::SeparatorText("Presets");
		for (unsigned int i = 0; i < _numPresets; i++) {
			if (ImGui::Selectable(_presets[i].title.c_str())) {
				selectedPreset = static_cast<int>(i);
			}
		}
		ImGui::EndPopup();
	}

	if (selectedPreset >= 0) {
		loadColorPreset(_presets[selectedPreset]);
	}

	auto span = _colors->gradientEndpoints.getSpan();

	bool modeChanged = ImGui::Checkbox("All Colors", &_colorMode);
	if (modeChanged) {
		if (_colorMode == GRAPH_COLOR_SINGLE) { // switched to single color mode
			_colors->setSingleColor(span[0]);
			_colors->colorChanged.set();
		}
	}

	if (_colorMode == GRAPH_COLOR_MULTI) { // multi color mode
		bool colorChanged = false;

		for (size_t i = 0; i < span.size(); i++) {
			// FIXME bad label
			colorChanged |= ImGui::ColorEdit3(
				_colorLabels.c_str(i),
				glm::value_ptr(span[i])
			);
		}

		if (colorChanged) {
			_colors->colorChanged.set();
		}
	} else {
		bool colorChanged = ImGui::ColorEdit3("Color", glm::value_ptr(span[0]));
		if (colorChanged) {
			_colors->setSingleColor(span[0]);
			_colors->colorChanged.set();
		}
	}
}
}
