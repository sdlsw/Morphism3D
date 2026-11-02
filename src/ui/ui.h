#pragma once

#include "event.h"
#include "generated/appinfo.h"
#include "status.h"
#include "temporal.h"
#include "ui/common.h"

#include <memory>
#include <vector>

namespace g3d {
class StatusBar {
private:
	static constexpr float defaultMessageTime = 5.0f;

	float _lastBarHeight = 20.0f;
	float _fpsReading = 0.0f;

	TimePoint _messageStart = now();
	float _messageSeconds = defaultMessageTime;
	std::string _message = "Welcome to " APPLICATION_NAME "!";

	void showBody();

	void handleStatusMessageEvent(const StatusMessageEvent& e);

	CompoundMethodEventHandler<StatusBar,
		StatusMessageEvent
	> _eventHandlers { this,
		std::mem_fn(&StatusBar::handleStatusMessageEvent)
	};
public:
	void show();
	void fpsReading(float fps) { _fpsReading = fps; }

	StatusBar(EventRouter& eventRouter) {
		_eventHandlers.addToRouter(eventRouter);
	}
};

class Ui {
private:
	std::vector<std::unique_ptr<UiWindow>> _windows;
	StatusBar _statusBar;

	void mainMenuBar();
	void windowMenu();

public:
	bool showDemoWindow = false;
	void show();

	Ui() = delete;
	Ui(Ui&&) = delete;
	Ui(const Ui&) = delete;

	Ui(EventRouter& eventRouter) : _statusBar { eventRouter } {}

	template<std::derived_from<UiWindow> W, typename... Args>
	void addWindow(Args&&... args) {
		auto window = std::make_unique<W>(std::forward<Args>(args)...);
		_windows.emplace_back(std::move(window));
	}

	auto& statusBar() { return _statusBar; }
};
}
