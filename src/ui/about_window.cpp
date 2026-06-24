#include "ui/about_window.h"

namespace g3d {
void AboutWindow::drawUi() {
	ImGui::Text("Morphism3D");
	ImGui::TextLinkOpenURL("Homepage", "https://github.com/sdlsw/Morphism3D");

	ImGui::Separator();

	ImGui::Text("(c) 2026 sdlsw");
	ImGui::Text("Morphism3D is licensed under AGPL-3.0; see ");
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextLinkOpenURL("LICENSE", "https://github.com/sdlsw/Morphism3D/blob/master/LICENSE");
}
}
