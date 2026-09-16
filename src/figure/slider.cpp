#include "figure/slider.h"

namespace g3d {
void SliderFigure::handleFigureRemoved(const FigureRemovedEvent& e) {
	if (e.id != _id) return;
	_varRange.variableStore().set(_varRange.var(), 0.0f);
}
}
