#include "ui/element/slider.h"

#include "ui/common_expression.h"

namespace g3d {
void SliderElement::updateStore() {
	auto& r = _figure->varRange();
	char v = r.var();
	if (v != '\0') {
		r.variableStore().set(v, _value);
	}
}

void SliderElement::show() {
	auto& range = _figure->varRange();

	// Top row
	bool varChanged = variableRangePanel(range, _guiIds);

	// Slider gets dedicated row
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool valChanged = ImGui::SliderFloat(
		_guiIds.c_str("##valueEntry"),
		&_value,
		range.min,
		range.max
	);

	if (varChanged || valChanged) updateStore();
}
}
