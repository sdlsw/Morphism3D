#include "ui/debug_window.h"

#include "expression.h"

namespace g3d {
void DebugWindow::tokenizerTestInput() {
	static std::array<char, 255> buf;
	ImGui::InputText("Tokenizer Test", buf.data(), buf.size());

	ImGui::SameLine();
	if (ImGui::Button("Run##Tokenize")) {
		tokenizerTest(buf.data());
	}
}

void DebugWindow::parserTestInput() {
	static std::array<char, 255> buf;
	ImGui::InputText("Parser Test", buf.data(), buf.size());

	ImGui::SameLine();
	if (ImGui::Button("Run##Parse")) {
		parserTest(buf.data());
	}
}

void DebugWindow::drawUi() {
	ImGui::Checkbox("Show Test Object", &_debugSettings->renderTestObject);
	tokenizerTestInput();
	parserTestInput();
}
}
