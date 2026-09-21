#include "ui/element/animator.h"

namespace g3d {
void AnimatorElement::updateTitle() {
	_title = std::format("Animator: {}", _figure->varRange().var());
}

void AnimatorElement::show() {
	auto& range = _figure->varRange();

	bool varChanged = variableRangePanel(range, _guiIds);
	if (varChanged) updateTitle();

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
