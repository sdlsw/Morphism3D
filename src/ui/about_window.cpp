#include "ui/about_window.h"

namespace g3d {
std::string AboutWindow::makeVersionString() {
	return std::format("{} v{}", APPLICATION_NAME, VERSION);
}

std::string AboutWindow::makeLicenseString() {
	return std::format("{} is licensed under AGPL-3.0; see ", APPLICATION_NAME);
}

void AboutWindow::dependency(
	const std::string& name,
	const std::string& link,
	const std::string& linkType
) {
	ImGui::Bullet();
	ImGui::TextLinkOpenURL(name.c_str(), link.c_str());
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Text(std::format(" - Linking: {}", linkType).c_str());
}

void AboutWindow::drawUi() {
	ImGui::Text(_versionString.c_str());
	ImGui::TextLinkOpenURL("Homepage", "https://github.com/sdlsw/Morphism3D");

	ImGui::Separator();

	ImGui::Text("(c) 2026 sdlsw");
	ImGui::Text(_licenseString.c_str());
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextLinkOpenURL("LICENSE", "https://github.com/sdlsw/Morphism3D/blob/master/LICENSE");

	if (ImGui::TreeNode("Credits")) {
		ImGui::Text("The following third-party libraries were used to make this application:");
		dependency("GLFW", "https://www.glfw.org/", "Static");
		dependency("GLM", "http://glm.g-truc.net/", "Static");
		dependency("Dear ImGui", "https://github.com/ocornut/imgui", "Static");
		dependency("stb_image.h", "https://github.com/nothings/stb/blob/master/stb_image.h", "Static");
		dependency("Vulkan SDK", "https://www.lunarg.com/products/vulkan-sdk/", "Dynamic");

		ImGui::TreePop();
	}
}
}
