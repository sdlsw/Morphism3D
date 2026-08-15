#include "ui/variable_window.h"

namespace g3d {
bool SliderElement::varValid(char c) {
	return isAlpha(c) && c != 't' && c != 'x' && c != 'y';
}

void SliderElement::updateStore() {
	char v = var();
	if (v != '\0') {
		_variableStore->set(v, _value);
	}
}

void SliderElement::show() {
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

char SliderElement::var() const {
	return _entryBuffer[0];
}

bool VariableWindow::hasSlider(char c) {
	for (const auto& [id, slider] : _sliders) {
		if (slider.var() == c) return true;
	}

	return false;
}

char VariableWindow::findFirstAvailableVar() {
	for (char c = 'a'; c <= 'z'; c++) {
		// NOTE: This is a bit slow (iterating over all sliders every
		// time we check) but in practice it doesn't seem to matter, so
		// stick with simpler algorithm
		if (!hasSlider(c) && SliderElement::varValid(c)) return c;
	}

	for (char c = 'A'; c <= 'Z'; c++) {
		if (!hasSlider(c) && SliderElement::varValid(c)) return c;
	}

	return '\0';
}

void VariableWindow::addSlider(char c) {
	_sliders.emplace(_nextSliderId, SliderElement(*_vars, _nextSliderId, c));
	auto& newSlider = _sliders.at(_nextSliderId);

	_panel.addFrame(newSlider);

	_nextSliderId++;
}

void VariableWindow::drawUi() {
	if (ImGui::Button("Add Slider")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addSlider(avail);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove All")) {
		_panel.removeAllFrames();
	}

	_panel.show();

	// Erase any sliders that were removed from the panel
	std::erase_if(_sliders, [this](const auto& item) {
		const auto& [id, slider] = item;
		if (!slider.shouldShow) {
			SliderRemovedEvent e { slider.id() };
			_vars->eventRouter().routeEvent(e);
			return true;
		}
		return false;
	});
}
}
