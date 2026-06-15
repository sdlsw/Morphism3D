#pragma once

#include "statistics.h"
#include "ui/common.h"

namespace g3d {
class StatsWindow : public UiWindow {
private:
	const std::string _title { "Stats" };

	TimerCollection* _timers;

	void drawTimer(const std::string& label, Timer& timer);

	void drawTimerFromCollection(const std::string& timerId);
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	StatsWindow() = delete;
	StatsWindow(TimerCollection& timers) : _timers { &timers } {
		open = false;
	}
};
}
