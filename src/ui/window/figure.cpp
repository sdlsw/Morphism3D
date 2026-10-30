#include "ui/window/figure.h"

#include "ui/element/all.h"

#include <typeinfo>

namespace g3d {
const AddFigureMenuEntry FigureWindow::addFigureMenu[] {
	{ "Surface Graph", std::mem_fn(addGraph<SurfaceGraphFigure>) },
	{ "Line Graph", std::mem_fn(addGraph<LineGraphFigure>) },
	{ "Slider", std::mem_fn(addVariableFigure<SliderFigure>) },
	{ "Animator", std::mem_fn(addVariableFigure<AnimatorFigure>) }
};

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

void FigureWindow::showAddFigureButton() {
	if (ImGui::Button("Add Figure...")) {
		ImGui::OpenPopup("add_figure_popup");
	}

	int selectedFigure = -1;
	if (ImGui::BeginPopup("add_figure_popup")) {
		ImGui::SeparatorText("Figures");
		for (unsigned int i = 0; i < std::size(addFigureMenu); i++) {
			if (ImGui::Selectable(addFigureMenu[i].name.c_str())) {
				selectedFigure = static_cast<int>(i);
			}
		}
		ImGui::EndPopup();
	}

	if (selectedFigure >= 0) {
		addFigureMenu[selectedFigure].callback(this);
	}
}

void FigureWindow::drawUi() {
	showAddFigureButton();
	ImGui::SameLine();
	if (ImGui::Button("Remove All")) {
		_panel.removeAllFrames();
	}

	_panel.show();

	// _figures.update() handles final removal of figures
}
}
