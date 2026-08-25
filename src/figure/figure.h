#pragma once

#include "event.h"
#include "figure/iface.h"
#include "ui/common.h"

#include <concepts>
#include <memory>

namespace g3d {
template<std::derived_from<MathFigure> F>
struct UiElementForFigure {
	typedef UiElement value;
};

class FigureCollection {
private:
	std::unordered_map<unsigned int, std::unique_ptr<MathFigure>> _figures;
	std::unordered_map<unsigned int, std::unique_ptr<UiElement>> _uiElements;
	unsigned int _nextId = 0;

	EventRouter* _eventRouter;

	unsigned int nextId();
public:
	FigureCollection(EventRouter& eventRouter) : _eventRouter { &eventRouter } {}

	const std::unordered_map<unsigned int, std::unique_ptr<MathFigure>>& figures() { return _figures; }

	template<std::derived_from<MathFigure> F, typename... Args>
	F& addFigure(Args&&... args) {
		unsigned int id = nextId();
		auto figure = std::make_unique<F>(id, std::forward<Args>(args)...);
		auto element = std::make_unique<typename UiElementForFigure<F>::value>(*figure.get());
		_figures.try_emplace(id, std::move(figure));
		_uiElements.try_emplace(id, std::move(element));

		return *dynamic_cast<F*>(_figures.at(id).get());
	}

	UiElement& getUiElement(unsigned int id);
	UiElement& getUiElement(const MathFigure& figure);

	void update();
	void updateSynchronized();
	void draw();
};
};
