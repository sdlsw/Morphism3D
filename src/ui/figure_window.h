#pragma once

#include "figure.h"
#include "expression.h"
#include "ui/common.h"
#include "window.h"

namespace g3d {
class FigureWindow : public UiWindow {
private:
	const std::string _title { "Figures" };
	RearrangeablePanel _panel;

	FigureCollection* _figures;
	VariableStore* _vars;

	unsigned int _nextSliderId = 0;

	// Returns true if this panel has a slider with the given character
	// defined.
	bool hasSlider(char c);

	// Adds a slider. Sliders are removed in the UI through an X on their
	// header bar.
	void addSlider(char c);

	// Finds an available variable that doesn't already have a slider.
	// Returns '\0' on failure.
	char findFirstAvailableVar();

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	FigureWindow(FigureCollection& figures, Window& window, VariableStore& vars)
	: _figures { &figures },
	  _panel { window },
	  _vars { &vars }
	{}
};
}
