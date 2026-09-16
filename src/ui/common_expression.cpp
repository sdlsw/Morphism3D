#include "ui/common_expression.h"

namespace g3d {
// Basic UI element for VariableRange. Covers min/max and variable name.
// Returns true when the variable has changed.
bool variableRangePanel(VariableRange& range, DiscriminatedStringMap& guiIds) {
	ImGui::AlignTextToFramePadding();
	ImGui::PushItemWidth(80);

	ImGui::InputFloat(guiIds.c_str("##varMinEntry"), &range.min);
	ImGui::SameLine();
	ImGui::Text("<=");

	ImGui::SameLine();
	ImGui::SetNextItemWidth(20);
	char lastVar = range.var();
	auto& varString = range.varString;
	bool varChanged = ImGui::InputText(guiIds.c_str("##varEntry"), varString.data(), varString.size());
	if (varChanged && !VariableRange::varValid(range.var())) {
		varString[0] = lastVar;
		varChanged = false;
	}

	if (range.var() != lastVar) {
		range.variableStore().set(lastVar, 0.0f);
	}

	ImGui::SameLine();
	ImGui::Text("<=");
	ImGui::SameLine();
	ImGui::InputFloat(guiIds.c_str("##varMaxEntry"), &range.max);

	ImGui::PopItemWidth();

	return varChanged;
}
}
