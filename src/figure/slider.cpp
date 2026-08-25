#include "figure/slider.h"

namespace g3d {
bool SliderFigure::varValid(char c) {
	return isAlpha(c) && c != 't' && c != 'x' && c != 'y';
}
}
