#include "ui/figure_slider_element.h"

namespace g3d {
void SliderElement::updateStore() {
	char v = _figure->var();
	if (v != '\0') {
		_figure->variableStore().set(v, _value);
	}
}

void SliderElement::show() {
	// Top row
	ImGui::AlignTextToFramePadding();
	ImGui::PushItemWidth(80);
	ImGui::InputFloat(_minEntryId.c_str(), &_figure->min());
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(20);
	char lastVar = _figure->var();
	auto& varString = _figure->varString();
	bool varChanged = ImGui::InputText(_varEntryId.c_str(), varString.data(), varString.size());
	if (varChanged && !SliderFigure::varValid(_figure->var())) {
		// Don't allow the user to enter an invalid variable name
		varString[0] = lastVar;
		varChanged = false;
	}

	if (_figure->var() != lastVar) {
		_figure->variableStore().set(lastVar, 0.0f);
	}

	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::InputFloat(_maxEntryId.c_str(), &_figure->max());
	ImGui::PopItemWidth();

	// Slider gets dedicated row
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool valChanged = ImGui::SliderFloat(
		_valueEntryId.c_str(),
		&_value,
		_figure->min(),
		_figure->max()
	);

	if (varChanged || valChanged) updateStore();
}
}
