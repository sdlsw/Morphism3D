#pragma once

#include "statistics.h"
#include "ui/common.h"
#include "ui/window/timer_config.h"

namespace g3d {
class PerformanceGraphElement : public UiElement {
private:
	std::string _timerName;
	unsigned int _id;

	TimerCollection* _timers;

	DiscriminatedStringMap _guiIds;
	
	TimerConfigWindow _configWindow;
public:
	const std::string& title() const override { return _timerName; }
	unsigned int id() const override { return _id; }
	void show() override;

	PerformanceGraphElement(
		TimerCollection& timers,
		const std::string& timerName,
		unsigned int id
	)
	: _timers { &timers },
	  _timerName { timerName },
	  _id { id },
	  _guiIds { std::format("{}", id) },
	  _configWindow { timers.getTimer(timerName), timerName }
	{}
};
}
