#include "figure/slider.h"

namespace g3d {
void SliderFigure::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;
	_variableStore->set(var(), 0.0f);
}

bool SliderFigure::varValid(char c) {
	return isAlpha(c) && c != 't' && c != 'x' && c != 'y';
}
}
