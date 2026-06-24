#pragma once

#include "ui/common.h"

namespace g3d {
class AboutWindow : public UiWindow {
private:
	const std::string _title { "About" };

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	AboutWindow() {
		open = false;
	}
};
}
