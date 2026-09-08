#include "ui/window/figure.h"

#include "ui/element/graph.h"
#include "ui/element/slider.h"

#include <typeinfo>

namespace g3d {
bool FigureWindow::hasSlider(char c) {
	const auto& sliderType = typeid(SliderFigure);

	for (const auto& [id, figure] : _figures->figures()) {
		if (typeid(*figure.get()) == sliderType) {
			const auto* slider = dynamic_cast<SliderFigure*>(figure.get());
			if (slider->var() == c) return true;
		}
	}

	return false;
}

char FigureWindow::findFirstAvailableVar() {
	for (char c = 'a'; c <= 'z'; c++) {
		// NOTE: This is a bit slow (iterating over all sliders every
		// time we check) but in practice it doesn't seem to matter, so
		// stick with simpler algorithm
		if (!hasSlider(c) && SliderFigure::varValid(c)) return c;
	}

	for (char c = 'A'; c <= 'Z'; c++) {
		if (!hasSlider(c) && SliderFigure::varValid(c)) return c;
	}

	return '\0';
}

void FigureWindow::addSlider(char c) {
	auto& newSlider = _figures->addFigure<SliderFigure>(*_vars, c);
	_panel.addFrame(_figures->getUiElement(newSlider));
}

void FigureWindow::addGraph() {
	// TODO: There might be a memory leak here, removing a GraphFigure
	// removes both halves of a dynamic buffer while one half is occupied,
	// causing the free call to fail.
	auto& newGraph = _figures->addFigure<GraphFigure>(
		_vars->eventRouter(),
		*_renderer,
		*_vars,
		80, // initial cells
		*_range,
		*_perfTimers,
		*_material
	);
	_panel.addFrame(_figures->getUiElement(newGraph));
}

void FigureWindow::drawUi() {
	if (ImGui::Button("Add Slider")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addSlider(avail);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Graph")) {
		addGraph();
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove All")) {
		_panel.removeAllFrames();
	}

	_panel.show();

	// _figures.update() handles final removal of figures
}
}
