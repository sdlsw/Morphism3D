#pragma once

#include "ui/common.h"
#include "vk/datatypes.h"

#include <string>

namespace g3d {
class RenderWindow : public UiWindow {
private:
	const std::string _title { "Render" };

	RenderSettings* _renderSettings;
	WithInitial<Light>* _light;
	WithInitial<Material>* _material;

	void basicSettingsSection();
	void lightSection();
	void materialSection();
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	RenderWindow() = delete;
	RenderWindow(
		RenderSettings& renderSettings,
		WithInitial<Light>& light,
		WithInitial<Material>& material
	)
	: _renderSettings { &renderSettings },
	  _light { &light },
	  _material { &material }
	{}
};
}
