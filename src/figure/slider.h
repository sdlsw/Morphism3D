#pragma once

#include "expression.h"
#include "figure/iface.h"

#include <string>

namespace g3d {
class SliderFigure : public MathFigure {
private:
	VariableStore* _variableStore;

	std::string _varString;

	void handleFigureRemoved(const FigureRemovedEvent& e);

	CompoundMethodEventHandler<SliderFigure,
		FigureRemovedEvent
	> _eventHandlers { this,
		std::mem_fn(handleFigureRemoved)
	};

	unsigned int _id;

	float _min = 0.0f;
	float _max = 3.0f;
public:
	static bool varValid(char c);

	SliderFigure(unsigned int id, VariableStore& variableStore, char v)
	: _id { id },
	  _varString { v, '\0' },
	  _variableStore { &variableStore }
	{
		_eventHandlers.addToRouter(variableStore.eventRouter());
	}

	SliderFigure(SliderFigure&& other)
	: _id { other._id },
	  _varString { std::move(other._varString) },
	  _variableStore { other._variableStore },
	  _eventHandlers { std::move(other._eventHandlers) }
	{
		_eventHandlers.updateThis(this);
	}

	VariableStore& variableStore() { return *_variableStore; }

	unsigned int id() const override { return _id; }

	char var() const { return _varString[0]; }
	std::string& varString() { return _varString; }

	float& min() { return _min; }
	float& max() { return _max; }
};
}
