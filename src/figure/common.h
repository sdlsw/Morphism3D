#pragma once

#include "expression.h"

namespace g3d {
// Utility class for a constrained variable. Used for both sliders and
// animators.
class VariableRange {
private:
	VariableStore* _variableStore;

public:
	static bool varValid(char c);

	std::string varString;

	float min;
	bool minLimited = true;
	float max;
	bool maxLimited = true;

	VariableRange(
		VariableStore& variableStore,
		char v,
		float min,
		float max
	)
	: _variableStore { &variableStore },
	  varString { v, '\0' },
	  min { min },
	  max { max }
	{}

	char var() const { return varString[0]; }
	VariableStore& variableStore() { return *_variableStore; }
};
}
