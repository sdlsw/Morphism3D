#include "ui/element/animator.h"

namespace g3d {
void AnimatorElement::updateTitle() {
	_title = std::format("Animator: {}", _figure->varRange().var());
}

void AnimatorElement::show() {
	auto& range = _figure->varRange();

	bool varChanged = variableRangePanel(range, _guiIds);
	if (varChanged) updateTitle();

	std::string pausedIndicator;
	if (_figure->paused) {
		pausedIndicator = " (paused)";
	}

	ImGui::Text(
		"%c = %.4f%s",
		range.var(),
		range.variableStore().get(range.var()),
		pausedIndicator.c_str()
	);

	ImGui::Checkbox(_guiIds.c_str("Loop"), &range.maxLimited);

	if (ImGui::Button(_guiIds.c_str("Reset"))) _figure->reset();

	ImGui::SameLine();
	if (_figure->paused) {
		if (ImGui::Button(_guiIds.c_str("Unpause"))) _figure->paused = false;
	} else {
		if (ImGui::Button(_guiIds.c_str("Pause"))) _figure->paused = true;
	}
}
}
