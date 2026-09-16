#include "figure/common.h"

namespace g3d {
bool VariableRange::varValid(char c) {
	return isAlpha(c) && c != 'x' && c != 'y';
}
}
