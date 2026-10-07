#include "ui/window/timer_config.h"

namespace g3d {
void TimerConfigWindow::drawUi() {
	int maxSize = static_cast<int>(_timer->measurements().maxSize());
	bool changed = ImGui::InputInt("Max Size", &maxSize);
	if (changed && maxSize > 0) {
		_timer->measurements().maxSize(static_cast<size_t>(maxSize));
	}
}
}
