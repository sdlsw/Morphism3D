#include "figure/figure.h"

namespace g3d {
unsigned int FigureCollection::nextId() {
	return _nextId++;
}

UiElement& FigureCollection::getUiElement(unsigned int id) {
	if (_uiElements.contains(id)) {
		return *_uiElements.at(id).get();
	} else {
		throw std::runtime_error("could not find UI element");
	}
}

UiElement& FigureCollection::getUiElement(const MathFigure& figure) {
	return getUiElement(figure.id());
}

void FigureCollection::update() {
	std::erase_if(_uiElements, [this](const auto& item) {
		const auto& [id, elem] = item;
		if (!elem.get()->shouldShow) {
			std::cerr << "FigureCollection: Removing " << id << std::endl;

			// TODO: Would a simpler onRemove() callback be better
			// here?
			FigureRemovedEvent e { id };
			_eventRouter->routeEvent(e);

			_figures.erase(id);

			return true;
		}

		return false;
	});

	for (auto& [_, figure] : _figures) {
		figure.get()->update();
	}
}

void FigureCollection::updateSynchronized() {
	for (auto& [_, figure] : _figures) {
		figure.get()->updateSynchronized();
	}
}

void FigureCollection::draw() {
	for (auto& [_, figure] : _figures) {
		figure.get()->draw();
	}
}
};
