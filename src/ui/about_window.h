#pragma once

#include "ui/common.h"
#include "generated/appinfo.h"

namespace g3d {
class AboutWindow : public UiWindow {
private:
	const std::string _title { "About" };
	std::string _versionString;
	std::string _licenseString;

	std::string makeVersionString();
	std::string makeLicenseString();

	void dependency(
		const std::string& name,
		const std::string& link,
		const std::string& linkType
	);
public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	AboutWindow()
	: _versionString { makeVersionString() },
	  _licenseString { makeLicenseString() }
	{
		open = false;
	}
};
}
