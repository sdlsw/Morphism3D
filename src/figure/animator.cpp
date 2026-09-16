#include "figure/animator.h"

namespace g3d {
void AnimatorFigure::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;
	_varRange.variableStore().set(_varRange.var(), 0.0f);
}

void AnimatorFigure::reset() {
	_startTime = now();
}

float AnimatorFigure::currentTime() {
	return secondsSince(_startTime);
}

void AnimatorFigure::update() {
	auto& r = _varRange;
	float value;

	if (_varRange.maxLimited) {
		value = glm::mod(currentTime() - r.min, r.max - r.min) + r.min;
	} else {
		value = currentTime();
	}

	_varRange.variableStore().set(_varRange.var(), value);
}
}
