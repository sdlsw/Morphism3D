#include "ui/figure_window.h"

#include "ui/figure_slider_element.h"

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

void FigureWindow::drawUi() {
	if (ImGui::Button("Add Slider")) {
		char avail = findFirstAvailableVar();
		if (avail != '\0') {
			addSlider(avail);
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
