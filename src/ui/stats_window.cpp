#include "stats_window.h"

float getTimerMeasurement(void* timer_ptr, int i) {
	g3d::Timer& timer = *reinterpret_cast<g3d::Timer*>(timer_ptr);
	auto& m = timer.measurements();

	// Display in reverse order so plots move from right to left
	return m.getMeasurement(m.size() - 1 - i);
}

namespace g3d {
void StatsWindow::drawTimer(const std::string& label, Timer& timer) {
	std::string avgOverlay = std::format("average: {:.4f}ms", timer.measurements().getAverage() * 1000);
	ImGui::PlotLines(
		label.c_str(),
		getTimerMeasurement,
		&timer,
		timer.measurements().size(),
		0,
		avgOverlay.c_str(),
		0.0f,
		FLT_MAX,
		ImVec2(0, 80.0f)
	);
}

void StatsWindow::drawTimerFromCollection(const std::string& timerId) {
	drawTimer(timerId, _timers->getTimer(timerId));
}

void StatsWindow::drawUi() {
	// TODO: Allow the user to select which of these get shown?
	drawTimerFromCollection("frame");
	drawTimerFromCollection("regen");
	drawTimerFromCollection("regenPositions");
	drawTimerFromCollection("regenNormals");
	drawTimerFromCollection("regenNormalPositions");
	drawTimerFromCollection("upload");
}
}
