#include "ui/window/stats.h"

namespace g3d {
void StatsWindow::addGraph(const std::string& timerName) {
	if (_elements.contains(timerName)) {
		_elements.at(timerName).shouldShow = true;
	} else {
		_elements.emplace(timerName, PerformanceGraphElement(
			*_timers,
			timerName,
			_nextId
		));
		_nextId++;
	}

	_panel.addFrame(_elements.at(timerName));
}

void StatsWindow::timerSelection() {
	for (const auto& [timerName, timer] : _timers->timers()) {
		bool selected = (
			_elements.contains(timerName) &&
			_elements.at(timerName).shouldShow
		);

		bool clicked = ImGui::Selectable(timerName.c_str(), selected);

		if (clicked) {
			if (selected) {
				_panel.removeFrame(_elements.at(timerName));
			} else {
				addGraph(timerName);
			}
		}
	}
}

void StatsWindow::drawUi() {
	// Before doing any UI stuff, wipe the elements and frames belonging to
	// any timer that no longer exists.
	std::erase_if(_elements, [this](const auto& item) {
		const auto& [timerName, element] = item;

		if (!_timers->hasTimer(timerName)) {
			if (element.shouldShow) _panel.removeFrame(element);
			return true;
		}

		return false;
	});

	ImGui::SeparatorText("Timer Selection");
	ImGui::BeginChild(
		"TimerSelection",
		ImVec2(ImGui::GetContentRegionAvail().x, 50),
		ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY,
		ImGuiWindowFlags_HorizontalScrollbar
	);
	timerSelection();
	ImGui::EndChild();

	ImGui::SeparatorText("Timer Graphs");
	if (_elements.size() == 0) {
		ImGui::TextWrapped(
			"Click some timer names under \"Timer Selection\" "
			"to show their graphs in this section."
		);
	} else {
		_panel.show();
	}
}
}
