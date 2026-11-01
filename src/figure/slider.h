#pragma once

#include "expression.h"
#include "figure/common.h"
#include "figure/iface.h"

#include <string>

namespace g3d {
class SliderFigure : public MathFigure {
private:
	VariableRange _varRange;

	void handleFigureRemoved(const FigureRemovedEvent& e);

	CompoundMethodEventHandler<SliderFigure,
		FigureRemovedEvent
	> _eventHandlers { this,
		std::mem_fn(&SliderFigure::handleFigureRemoved)
	};

	unsigned int _id;
public:
	SliderFigure(unsigned int id, VariableStore& variableStore, char v)
	: _id { id },
	  _varRange { variableStore, v, 0.0f, 3.0f }
	{
		_eventHandlers.addToRouter(variableStore.eventRouter());
	}

	SliderFigure(SliderFigure&& other)
	: _id { other._id },
	  _varRange { other._varRange },
	  _eventHandlers { std::move(other._eventHandlers) }
	{
		_eventHandlers.updateThis(this);
	}

	unsigned int id() const override { return _id; }
	VariableRange& varRange() { return _varRange; }
};
}
