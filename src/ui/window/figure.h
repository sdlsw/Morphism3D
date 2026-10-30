#pragma once

#include "expression.h"
#include "figure/figure.h"
#include "range.h"
#include "settings.h"
#include "statistics.h"
#include "ui/common.h"
#include "vk/renderer.h"
#include "window.h"

namespace g3d {
struct AddFigureMenuEntry;

class FigureWindow : public UiWindow {
private:
	static const AddFigureMenuEntry addFigureMenu[];

	const std::string _title { "Figures" };

	RearrangeablePanel _panel;

	FigureCollection* _figures;
	VariableStore* _vars;
	Renderer* _renderer;
	TimerCollection* _perfTimers;
	Range* _range;
	DebugSettings* _debugSettings;

	// Returns true if this panel has a slider/animator with the given character
	// defined.
	bool varTaken(char c);

	template<typename T>
	void addGraph();

	template<typename T>
	void addVariableFigure();

	// Finds an available variable that doesn't already have a slider.
	// Returns '\0' on failure.
	char findFirstAvailableVar();

	void showAddFigureButton();
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
		DebugSettings& debugSettings
	)
	: _figures { &figures },
	  _panel { window },
	  _vars { &vars },
	  _renderer { &renderer },
	  _perfTimers { &perfTimers },
	  _range { &range },
	  _debugSettings { &debugSettings }
	{}
};

struct AddFigureMenuEntry {
	std::string name;
	std::function<void(g3d::FigureWindow*)> callback;
};

template<typename T>
void FigureWindow::addVariableFigure() {
	char avail = findFirstAvailableVar();
	if (avail != '\0') {
		auto& newFigure = _figures->addFigure<T>(*_vars, avail);
		_panel.addFrame(_figures->getUiElement(newFigure));
	}
}

template<typename T>
void FigureWindow::addGraph() {
	// TODO: There might be a memory leak here, removing a GraphFigure
	// removes both halves of a dynamic buffer while one half is occupied,
	// causing the free call to fail.
	auto& newGraph = _figures->addFigure<T>(
		_vars->eventRouter(),
		*_renderer,
		*_vars,
		80, // initial cells
		*_range,
		*_perfTimers,
		*_debugSettings
	);
	_panel.addFrame(_figures->getUiElement(newGraph));
}
}
