#pragma once

#include "expression.h"
#include "figure_iface.h"

#include <string>

namespace g3d {
class SliderFigure : public MathFigure {
private:
	VariableStore* _variableStore;

	std::string _varString;

	class _FigureRemovedHandler : public EventHandler<FigureRemovedEvent> {
	public:
		SliderFigure* _this;

		void handle(const FigureRemovedEvent& e) override {
			if (e.id != _this->_id) return;
			_this->_variableStore->set(_this->var(), 0.0f);
		}

		_FigureRemovedHandler(SliderFigure* figure) : _this { figure } {}
	};

	unsigned int _id;

	float _min = 0.0f;
	float _max = 3.0f;

	_FigureRemovedHandler _figureRemovedHandler { this };
public:
	static bool varValid(char c);

	SliderFigure(unsigned int id, VariableStore& variableStore, char v)
	: _id { id },
	  _varString { v, '\0' },
	  _variableStore { &variableStore }
	{
		variableStore.eventRouter().addHandler(_figureRemovedHandler);
	}

	SliderFigure(SliderFigure&& other)
	: _id { other._id },
	  _varString { std::move(other._varString) },
	  _variableStore { other._variableStore },
	  _figureRemovedHandler { std::move(other._figureRemovedHandler) }
	{
		_figureRemovedHandler._this = this;
	}

	VariableStore& variableStore() { return *_variableStore; }

	unsigned int id() const override { return _id; }

	char var() const { return _varString[0]; }
	std::string& varString() { return _varString; }

	float& min() { return _min; }
	float& max() { return _max; }
};
}
