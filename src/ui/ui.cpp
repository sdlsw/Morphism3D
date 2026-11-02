#include "ui/ui.h"

namespace g3d {
void StatusBar::handleStatusMessageEvent(const StatusMessageEvent& e) {
	_messageSeconds = e.overrideTime < 0.0f ? defaultMessageTime : e.overrideTime;
	_messageStart = now();
	_message = e.message();
}

void StatusBar::showBody() {
	ImGui::Text("%.2f FPS", _fpsReading);

	if (secondsSince(_messageStart) < _messageSeconds) {
		ImGui::SameLine();

		ImVec2 currentCursor = ImGui::GetCursorScreenPos();
		float messageWidth = ImGui::CalcTextSize(_message.c_str()).x;
		float availWidth = ImGui::GetContentRegionAvail().x;
		ImVec2 newCursor {
			currentCursor.x + availWidth - messageWidth,
			currentCursor.y
		};
		ImGui::SetCursorScreenPos(newCursor);

		ImGui::Text(_message.c_str());
	}
}

void StatusBar::show() {
	auto windowFlags = (
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus
	);

	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImVec2 barPos { 0, viewport->Size.y - _lastBarHeight };
	ImVec2 barSize { viewport->Size.x, _lastBarHeight };
	ImGui::SetNextWindowPos(barPos);
	ImGui::SetNextWindowSize(barSize);

	ImGui::PushStyleVarY(ImGuiStyleVar_WindowPadding, 1.0f);
	ImGui::Begin("##StatusBar", nullptr, windowFlags);
	ImVec2 start = ImGui::GetCursorScreenPos();
	showBody();
	ImVec2 end = ImGui::GetCursorScreenPos();
	_lastBarHeight = end.y - start.y + 2*ImGui::GetStyle().WindowPadding.y;
	ImGui::End();
	ImGui::PopStyleVar();
}

void Ui::show() {
	mainMenuBar();

	if (showDemoWindow) {
		ImGui::ShowDemoWindow(&showDemoWindow);
	}

	for (auto& window : _windows) {
		window.get()->show();
	}

	_statusBar.show();
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
