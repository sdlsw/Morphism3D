#pragma once

#include "expression.h"
#include "figure/figure.h"
#include "range.h"
#include "statistics.h"
#include "ui/common.h"
#include "vk/renderer.h"
#include "window.h"

namespace g3d {
class FigureWindow : public UiWindow {
private:
	const std::string _title { "Figures" };
	RearrangeablePanel _panel;

	FigureCollection* _figures;
	VariableStore* _vars;
	Renderer* _renderer;
	TimerCollection* _perfTimers;
	Range* _range;
	Material* _material;

	unsigned int _nextSliderId = 0;

	// Returns true if this panel has a slider/animator with the given character
	// defined.
	bool varTaken(char c);

	void addGraph();
	void addSlider(char c);
	void addAnimator(char c);

	// Finds an available variable that doesn't already have a slider.
	// Returns '\0' on failure.
	char findFirstAvailableVar();

public:
	const std::string& title() const override { return _title; }
	void drawUi() override;

	FigureWindow(
		FigureCollection& figures,
		Window& window,
		VariableStore& vars,
		Renderer& renderer,
		TimerCollection& perfTimers,
		Range& range,
		Material& material
	)
	: _figures { &figures },
	  _panel { window },
	  _vars { &vars },
	  _renderer { &renderer },
	  _perfTimers { &perfTimers },
	  _range { &range },
	  _material { &material }
	{}
};
}
