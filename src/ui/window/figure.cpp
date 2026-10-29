#include "ui/window/figure.h"

#include "ui/element/all.h"

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

void FigureWindow::drawUi() {
	if (ImGui::Button("Add Surface Graph")) {
		addGraph<SurfaceGraphFigure>();
	}

	ImGui::SameLine();
	if (ImGui::Button("Add Line Graph")) {
		addGraph<LineGraphFigure>();
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
