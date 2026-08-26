#include "ui/window/range.h"

namespace g3d {
void RangeWindow::rangeInput(char dim, float& low, float& high) {
	float lastLow = low;
	bool lowChanged = ImGui::InputFloat(std::format("##rangelow{}", dim).c_str(), &low);
	if (lowChanged) {
		if (low >= high) {
			low = lastLow;
		} else {
			_range->low(_rangeLow);
		}
	}

	char dimStr[] { dim, '\0' };
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::Text(dimStr);
	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();

	float lastHigh = high;
	bool highChanged = ImGui::InputFloat(std::format("##rangehigh{}", dim).c_str(), &high);
	if (highChanged) {
		if (high <= low) {
			high = lastHigh;
		} else {
			_range->high(_rangeHigh);
		}
	}
}

void RangeWindow::rangeInputs() {
	char dims[] { 'X', 'Y', 'Z' };

	ImGui::PushItemWidth(100.0f);
	for (unsigned int i = 0; i < 3; i++) {
		rangeInput(dims[i], _rangeLow[i], _rangeHigh[i]);
	}
	ImGui::PopItemWidth();
}

void RangeWindow::drawUi() {
	rangeInputs();
}
}
