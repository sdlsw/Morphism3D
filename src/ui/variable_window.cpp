#include "ui/variable_window.h"

namespace g3d {
bool VariableSlider::varValid(char c) {
	return isAlpha(c) && c != 't' && c != 'x' && c != 'y';
}

void VariableSlider::updateStore() {
	char v = var();
	if (v != '\0') _variableStore->set(v, _value);
}

bool VariableSlider::showHeader() {
	_upPressed = ImGui::ArrowButton(_upArrowId.c_str(), ImGuiDir_Up);
	ImGui::SameLine(0.0f, 0.0f);
	_downPressed = ImGui::ArrowButton(_downArrowId.c_str(), ImGuiDir_Down);
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
	ImGui::InputFloat(_minEntryId.c_str(), &_min);
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(20);
	char lastVar = var();
	bool varChanged = ImGui::InputText(_varEntryId.c_str(), _entryBuffer.data(), _entryBuffer.size());
	if (varChanged && !varValid(var())) {
		// Don't allow the user to enter an invalid variable name
		_entryBuffer[0] = lastVar;
		varChanged = false;
	}

	if (var() != lastVar) {
		_variableStore->set(lastVar, 0.0f);
	}

	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::InputFloat(_maxEntryId.c_str(), &_max);
	ImGui::PopItemWidth();

	// Slider gets dedicated row
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool valChanged = ImGui::SliderFloat(_valueEntryId.c_str(), &_value, _min, _max);

	if (varChanged || valChanged) updateStore();
}

void VariableSlider::onAdd() {
	updateStore();
}

void VariableSlider::onRemove() {
	_value = 0.0f;
	updateStore();
}

char VariableSlider::var() const {
	return _entryBuffer[0];
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

	_heights[slider.id()] = static_cast<unsigned int>(end.y - start.y);

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

	if (slider.upPressed() && i > 0) {
		_rearrangeFrom = i;
		_rearrangeTo = i-1;
		_rearrange = true;

		// Make cursor follow the up/down arrows, so you can click them
		// many times in succession to move items quickly
		auto& slider = getSlider(i-1);
		double yinc = -static_cast<double>(_heights[slider.id()]);
		_window->incCursorPosition(0, yinc);
	}

	if (slider.downPressed() && i < _sliderOrder.size() - 1) {
		_rearrangeFrom = i;
		_rearrangeTo = i+1;
		_rearrange = true;

		auto& slider = getSlider(i+1);
		double yinc = static_cast<double>(_heights[slider.id()]);
		_window->incCursorPosition(0, yinc);
	}
}

void SliderPanel::showSliders() {
	// Handle slider removal
	std::erase_if(_sliderOrder, [this](size_t id) {
		auto& slider = _sliders.at(id);

		if (!slider.exists) {
			slider.onRemove();
			_sliders.erase(id);
			_heights.erase(id);
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
	_sliders.emplace(_nextSliderId, VariableSlider(*_vars, _nextSliderId, c));
	_heights[_nextSliderId] = 0;
	_sliderOrder.push_back(_nextSliderId);

	// New slider immediately sets a value, so need to make sure graph gets
	// updated. FIXME assuming the added slider var is valid!
	auto& newSlider = _sliders.at(_nextSliderId);
	newSlider.onAdd();

	_nextSliderId++;
}

void VariableWindow::drawUi() {
	_sliders.show();
}
}
