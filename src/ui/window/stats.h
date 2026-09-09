#pragma once

#include "statistics.h"
#include "ui/common.h"
#include "ui/element/performance_graph.h"

namespace g3d {
class StatsWindow : public UiWindow {
private:
	const std::string _title { "Stats" };

	unsigned int _nextId = 0;

	std::unordered_map<std::string, PerformanceGraphElement> _elements;
	RearrangeablePanel _panel;

	TimerCollection* _timers;

	void addGraph(const std::string& timerName);
	void timerSelection();
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	StatsWindow() = delete;
	StatsWindow(TimerCollection& timers, Window& window)
	: _timers { &timers },
	  _panel { window }
	{
		open = false;
	}
};
}
