#include "ui/element/slider.h"

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
	ImGui::InputFloat(_guiIds.c_str("##minEntry"), &_figure->min());
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(20);
	char lastVar = _figure->var();
	auto& varString = _figure->varString();
	bool varChanged = ImGui::InputText(_guiIds.c_str("##varEntry"), varString.data(), varString.size());
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
	ImGui::InputFloat(_guiIds.c_str("##maxEntry"), &_figure->max());
	ImGui::PopItemWidth();

	// Slider gets dedicated row
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool valChanged = ImGui::SliderFloat(
		_guiIds.c_str("##valueEntry"),
		&_value,
		_figure->min(),
		_figure->max()
	);

	if (varChanged || valChanged) updateStore();
}
}
