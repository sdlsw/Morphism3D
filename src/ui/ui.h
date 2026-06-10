#pragma once

#include "ui/common.h"

#include <memory>
#include <vector>

namespace g3d {
class Ui {
private:
	std::vector<std::unique_ptr<UiWindow>> _windows;

	void mainMenuBar();
	void windowMenu();

public:
	bool showDemoWindow = false;
	void show();

	Ui() = default;
	Ui(Ui&&) = delete;
	Ui(const Ui&) = delete;

	template<std::derived_from<UiWindow> W, typename... Args>
	void addWindow(Args&&... args) {
		auto window = std::make_unique<W>(std::forward<Args>(args)...);
		_windows.emplace_back(std::move(window));
	}
};
}
