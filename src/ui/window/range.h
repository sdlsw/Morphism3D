#pragma once

#include <range.h>
#include "ui/common.h"

namespace g3d {
class RangeWindow : public UiWindow {
private:
	const std::string _title { "Range" };

	Range* _range;

	// Need to duplicate range state here due to validation of range inputs
	glm::vec3 _rangeLow;
	glm::vec3 _rangeHigh;

	void rangeInput(char dim, float& low, float& high);
	void rangeInputs();

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	RangeWindow(Range& range)
	: _range { &range },
	  _rangeLow { range.low() },
	  _rangeHigh { range.high() }
	{}
};
}
