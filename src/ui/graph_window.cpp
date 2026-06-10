#include "ui/graph_window.h"

static const std::string RENDER_MODE_SURFACE_NAME = "Surface";
static const std::string RENDER_MODE_WIREFRAME_NAME = "Wireframe";
static const std::string RENDER_MODE_NONE_NAME = "None";
static const std::string RENDER_MODE_UNKNOWN_NAME { g3d::UI_UNKNOWN_MODE_NAME };

namespace g3d {
const std::string& renderModeName(const GraphRenderMode& mode) {
	switch (mode) {
		case GraphRenderMode::surface:
			return RENDER_MODE_SURFACE_NAME;
		case GraphRenderMode::wireframe:
			return RENDER_MODE_WIREFRAME_NAME;
		case GraphRenderMode::none:
			return RENDER_MODE_NONE_NAME;
		default:
			return RENDER_MODE_UNKNOWN_NAME;
	}
}

bool VariableSlider::varValid(char c) {
	return isAlpha(c) && c != 't' && c != 'x' && c != 'y';
}

bool VariableSlider::showHeader() {
	upPressed = ImGui::ArrowButton(upArrowId.c_str(), ImGuiDir_Up);
	ImGui::SameLine(0.0f, 0.0f);
	downPressed = ImGui::ArrowButton(downArrowId.c_str(), ImGuiDir_Down);
	ImGui::SameLine();

	return ImGui::CollapsingHeader(
		headerId().c_str(),
		&exists,
		(
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_OpenOnArrow
		)
	);
}

void VariableSlider::showBody() {
	// Top row
	ImGui::AlignTextToFramePadding();
	ImGui::PushItemWidth(80);
	ImGui::InputFloat(minEntryId.c_str(), &min);
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(20);
	char lastVar = var();
	// FIXME If the variable is changed, say, a -> c, then a won't get
	// zeroed out.
	bool varChanged = ImGui::InputText(varEntryId.c_str(), entryBuffer.data(), entryBuffer.size());
	if (varChanged && !varValid(var())) {
		// Don't allow the user to enter an invalid variable name
		entryBuffer[0] = lastVar;
		varChanged = false;
	}
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::InputFloat(maxEntryId.c_str(), &max);
	ImGui::PopItemWidth();

	// Slider gets dedicated row
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool valChanged = ImGui::SliderFloat(valueEntryId.c_str(), &value, min, max);
	changed = varChanged || valChanged;
}

char VariableSlider::var() const {
	return entryBuffer[0];
}

VariableSlider& SliderPanel::getSlider(size_t i) {
	return _sliders.at(_sliderOrder[i]);
}

void SliderPanel::showSlider(size_t i) {
	auto& slider = getSlider(i);

	ImVec2 start = ImGui::GetCursorScreenPos();
	ImGui::BeginGroup();
	bool headerShown = slider.showHeader();

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoHoldToOpenOthers)) {
		ImGui::SetDragDropPayload("SLIDER_REARRANGE", &i, sizeof(size_t));
		ImGui::Text(std::format("Moving {}", slider.var()).c_str());
		ImGui::EndDragDropSource();
	}

	if (headerShown) slider.showBody();
	ImGui::EndGroup();
	ImVec2 end = ImGui::GetCursorScreenPos();

	slider.height = static_cast<unsigned int>(end.y - start.y);

	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
			"SLIDER_REARRANGE",
			ImGuiDragDropFlags_AcceptBeforeDelivery |
			ImGuiDragDropFlags_AcceptNoPreviewTooltip
		);

		if (payload) {
			ImGui::SetTooltip(std::format("Move to {}", slider.var()).c_str());
			if (payload->IsDelivery()) {
				_rearrangeFrom = *reinterpret_cast<const size_t*>(payload->Data);
				_rearrangeTo = i;
				_rearrange = true;
			}
		}

		ImGui::EndDragDropTarget();
	}

	if (slider.upPressed && i > 0) {
		_rearrangeFrom = i;
		_rearrangeTo = i-1;
		_rearrange = true;

		// Make cursor follow the up/down arrows, so you can click them
		// many times in succession to move items quickly
		double yinc = -static_cast<double>(getSlider(i-1).height);
		_window->incCursorPosition(0, yinc);
	}

	if (slider.downPressed && i < _sliderOrder.size() - 1) {
		_rearrangeFrom = i;
		_rearrangeTo = i+1;
		_rearrange = true;

		double yinc = static_cast<double>(getSlider(i+1).height);
		_window->incCursorPosition(0, yinc);
	}

	if (slider.changed) {
		_vars->set(slider.var(), slider.value);
		_changedVar = true;
	}
}

void SliderPanel::showSliders() {
	// Handle slider removal
	std::erase_if(_sliderOrder, [this](size_t id) {
		auto& slider = _sliders.at(id);

		if (!slider.exists) {
			_vars->set(slider.var(), 0.0f);
			_changedVar = true;
			_sliders.erase(id);
			return true;
		}

		return false;
	});

	for (size_t i = 0; i < _sliderOrder.size(); i++) {
		showSlider(i);
	}

	if (_rearrange) {
		int from = static_cast<int>(_rearrangeFrom);
		int to = static_cast<int>(_rearrangeTo);
		if (std::abs(from - to) == 1) {
			// slightly more efficient than erase/insert
			std::swap(_sliderOrder[_rearrangeFrom], _sliderOrder[_rearrangeTo]);
		} else {
			auto id = _sliderOrder[_rearrangeFrom];
			_sliderOrder.erase(_sliderOrder.begin() + _rearrangeFrom);
			_sliderOrder.insert(_sliderOrder.begin() + _rearrangeTo, id);
		}

		_rearrange = false;
	}
}

bool SliderPanel::hasSlider(char c) {
	for (const auto& [id, slider] : _sliders) {
		if (slider.var() == c) return true;
	}

	return false;
}

char SliderPanel::findFirstAvailableVar() {
	for (char c = 'a'; c <= 'z'; c++) {
		// NOTE: This is a bit slow (iterating over all sliders every
		// time we check) but in practice it doesn't seem to matter, so
		// stick with simpler algorithm
		if (!hasSlider(c) && VariableSlider::varValid(c)) return c;
	}

	for (char c = 'A'; c <= 'Z'; c++) {
		if (!hasSlider(c) && VariableSlider::varValid(c)) return c;
	}

	return '\0';
}

void SliderPanel::show() {
	if (ImGui::Button("Add Slider")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addSlider(avail);
			_changedVar = true;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove All")) {
		for (auto& [_, slider] : _sliders) {
			slider.exists = false;
		}
	}

	showSliders();
}

void SliderPanel::addSlider(char c) {
	_sliders.emplace(_nextSliderId, VariableSlider(_nextSliderId, c));
	_sliderOrder.push_back(_nextSliderId);

	// New slider immediately sets a value, so need to make sure graph gets
	// updated. FIXME assuming the added slider var is valid!
	auto& newSlider = _sliders.at(_nextSliderId);
	_vars->set(newSlider.var(), newSlider.value);
	_changedVar = true;

	_nextSliderId++;
}

bool SliderPanel::consumeChangedFlag() {
	bool f = _changedVar;
	if (_changedVar) _changedVar = false;
	return f;
}
}
