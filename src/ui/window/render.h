#pragma once

#include "ui/common_render.h"

#include <string>

namespace g3d {
class RenderWindow : public UiWindow {
private:
	const std::string _title { "Render" };

	RenderSettings* _renderSettings;
	WithInitial<Light>* _light;

	void basicSettingsSection();
	void lightSection();
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	RenderWindow() = delete;
	RenderWindow(
		RenderSettings& renderSettings,
		WithInitial<Light>& light
	)
	: _renderSettings { &renderSettings },
	  _light { &light }
	{}
};
}
