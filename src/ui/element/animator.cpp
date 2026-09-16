#include "ui/element/animator.h"

namespace g3d {
void AnimatorElement::show() {
	auto& range = _figure->varRange();

	variableRangePanel(range, _guiIds);

	ImGui::Text(std::format(
		"{} = {}",
		range.var(),
		range.variableStore().get(range.var())
	).c_str());

	ImGui::Checkbox(_guiIds.c_str("Loop"), &range.maxLimited);

	if (ImGui::Button(_guiIds.c_str("Reset"))) {
		_figure->reset();
	}
}
}
