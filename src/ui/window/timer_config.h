#pragma once

#include "statistics.h"
#include "ui/common.h"

namespace g3d {
class TimerConfigWindow : public UiWindow {
private:
	std::string _title;
	Timer* _timer;

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	TimerConfigWindow(Timer& timer, const std::string& name)
	: _timer { &timer },
	  _title { std::format("Timer Configuration: {}", name) }
	{}
};
}
