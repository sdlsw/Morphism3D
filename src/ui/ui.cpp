#include "ui/ui.h"

namespace g3d {
void Ui::show() {
	mainMenuBar();

	if (showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	for (auto& window : _windows) {
		window.get()->show();
	}
}

void Ui::mainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		windowMenu();
		ImGui::EndMainMenuBar();
	}
}

void Ui::windowMenu() {
	if (ImGui::BeginMenu("Window")) {
		for (auto& window : _windows) {
			auto* ptr = window.get();
			ImGui::MenuItem(ptr->title().c_str(), nullptr, &ptr->open);
		}
		ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
		ImGui::EndMenu();
	}
}
}
