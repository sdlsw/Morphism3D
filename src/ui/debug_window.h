#pragma once

#include <ui/common.h>

#include <string>

namespace g3d {
class DebugWindow : public UiWindow {
private:
	const std::string _title { "Debug" };

	DebugSettings* _debugSettings;

	void tokenizerTestInput();
	void parserTestInput();
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	DebugWindow() = delete;
	DebugWindow(DebugSettings& debugSettings)
	: _debugSettings { &debugSettings }
	{
		open = false;
	}
};
}
