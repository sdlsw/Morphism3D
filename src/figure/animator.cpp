#include "figure/animator.h"

namespace g3d {
void AnimatorFigure::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;
	_varRange.variableStore().set(_varRange.var(), 0.0f);
}

void AnimatorFigure::reset() {
	_rawValue = 0.0f;
}

void AnimatorFigure::update() {
	float deltaTime = secondsSince(_lastFrameTime);
	_lastFrameTime = now();

	if (paused) return;

	_rawValue += deltaTime*speed;

	auto& r = _varRange;
	float value;
	if (r.maxLimited) {
		value = glm::mod(_rawValue - r.min, r.max - r.min) + r.min;
	} else {
		value = _rawValue;
	}

	r.variableStore().set(r.var(), value);
}
}
