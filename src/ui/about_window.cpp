#include "ui/about_window.h"

namespace g3d {
std::string AboutWindow::makeVersionString() {
	return std::format("{} v{}", APPLICATION_NAME, VERSION);
}

std::string AboutWindow::makeLicenseString() {
	return std::format("{} is licensed under AGPL-3.0; see ", APPLICATION_NAME);
}

void AboutWindow::drawUi() {
	ImGui::Text(_versionString.c_str());
	ImGui::TextLinkOpenURL("Homepage", "https://github.com/sdlsw/Morphism3D");

	ImGui::Separator();

	ImGui::Text("(c) 2026 sdlsw");
	ImGui::Text(_licenseString.c_str());
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextLinkOpenURL("LICENSE", "https://github.com/sdlsw/Morphism3D/blob/master/LICENSE");
}
}
