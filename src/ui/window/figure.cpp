#include "ui/window/figure.h"

#include "ui/element/animator.h"
#include "ui/element/graph.h"
#include "ui/element/slider.h"

#include <typeinfo>

namespace g3d {
bool FigureWindow::varTaken(char c) {
	const auto& sliderType = typeid(SliderFigure);
	const auto& animatorType = typeid(AnimatorFigure);

	for (auto& [id, figure] : _figures->figures()) {
		if (typeid(*figure.get()) == sliderType) {
			auto* slider = dynamic_cast<SliderFigure*>(figure.get());
			if (slider->varRange().var() == c) return true;
		} else if (typeid(*figure.get()) == animatorType) {
			auto* animator = dynamic_cast<AnimatorFigure*>(figure.get());
			if (animator->varRange().var() == c) return true;
		}
	}

	return false;
}

char FigureWindow::findFirstAvailableVar() {
	for (char c = 'a'; c <= 'z'; c++) {
		// NOTE: This is a bit slow (iterating over all figures every
		// time we check) but in practice it doesn't seem to matter, so
		// stick with simpler algorithm
		if (!varTaken(c) && VariableRange::varValid(c)) return c;
	}

	for (char c = 'A'; c <= 'Z'; c++) {
		if (!varTaken(c) && VariableRange::varValid(c)) return c;
	}

	return '\0';
}

void FigureWindow::addAnimator(char c) {
	auto& newAnimator = _figures->addFigure<AnimatorFigure>(*_vars, c);
	_panel.addFrame(_figures->getUiElement(newAnimator));
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
	if (ImGui::Button("Add Graph")) {
		addGraph();
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Slider")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addSlider(avail);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Animator")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addAnimator(avail);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove All")) {
		_panel.removeAllFrames();
	}

	_panel.show();

	// _figures.update() handles final removal of figures
}
}
