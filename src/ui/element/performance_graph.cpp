#include "ui/element/performance_graph.h"

float getTimerMeasurement(void* timer_ptr, int i) {
	g3d::Timer& timer = *reinterpret_cast<g3d::Timer*>(timer_ptr);
	auto& m = timer.measurements();

	// Display in reverse order so plots move from right to left
	return m.getMeasurement(m.size() - 1 - i);
}

void drawTimer(const std::string& label, g3d::Timer& timer) {
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
		ImVec2(ImGui::GetContentRegionAvail().x, 80.0f)
	);
}

namespace g3d {
void PerformanceGraphElement::show() {
	drawTimer(_plotLabel, _timers->getTimer(_timerName));
}
}
